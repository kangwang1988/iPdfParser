//
//  PhasesViewController.m
//  iPdfParser
//
//  Created by KyleWong on 14-10-4.
//  Copyright (c) 2014年 kylewong. All rights reserved.
//

#import "PhasesViewController.h"
#import "context.h"
#import "Tesseract.h"
#import "UIImage+PDF.h"
#import "CustomIndicator.h"
#import "WordFragmentOCRCell.h"

#define PdfReflowSourceImageWidth 320*2
#define PdfReflowSourceImageHeight 480*2

#define kAlertViewPdfTag 1000
#define kAlertViewLangTag 1001

typedef enum{
    PDFParserOptionPdf,
    PDFParserOptionImage,
    PDFParserOptionReflow,
    PDFParserOptionSplitter,
    PDFParserOptionOCR
}PDFParserOption;

#define kAnimationDuration 0.35f

@interface PhasesViewController ()<UIActionSheetDelegate,UIScrollViewDelegate,UITableViewDataSource,UITableViewDelegate,WordFragmentCellDelegate>
@property (nonatomic,retain) NSString *documentDir;
@property (nonatomic,retain) NSString *prefixDir;
@property (nonatomic,retain) NSString *filename;
@property (nonatomic,retain) NSString *renderedImgPath;
@property (nonatomic,retain) NSString *reflowImgPath;
@property (nonatomic,retain) NSString *tesseractLang;
@property (nonatomic,retain) NSString *tesseractOCRText;
@property (nonatomic,assign) CGFloat zoomlevel;
@property (nonatomic,retain) NSNumber *rotationAngle;
@property (nonatomic,assign) NSInteger wordFragment_count;
@property (nonatomic,assign) PDFParserOption option;
@property (assign, nonatomic) IBOutlet UIView *pdfOptionsView;
@property (assign, nonatomic) IBOutlet UILabel *pdfDescLabel;
@property (assign, nonatomic) IBOutlet UIImageView *previewImgView;
@property (assign, nonatomic) IBOutlet UIImageView *mupdfImgView;
@property (assign, nonatomic) IBOutlet UIScrollView *reflowScrollView;
@property (retain, nonatomic) IBOutlet UIImageView *reflowImgView;
@property (retain, nonatomic) IBOutlet UITableView *splitterTableView;
@property (assign, nonatomic) IBOutlet UITextView *ocrTextView;
@property (assign, nonatomic) IBOutlet UIView *pdfOptionMaskView;
@property (assign, nonatomic) IBOutlet UISwitch *reflowHorizSwitch;
@property (nonatomic,assign) IBOutlet UIActivityIndicatorView *loadingView;
@property (nonatomic,retain) NSMutableDictionary *splitterOCRDict;
@property (nonatomic,assign) BOOL isInAnimation;
@property (nonatomic,assign) NSInteger pdfPageCount;
@property (nonatomic,assign) NSInteger curPage;
@property (nonatomic,assign) dispatch_queue_t syncQueue;
@end

@implementation PhasesViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        _documentDir = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] retain];
        _curPage = 1;
        _zoomlevel = 225;
        _rotationAngle = 0;
        _syncQueue = dispatch_queue_create("pdfParser", NULL);
        _splitterOCRDict = [[NSMutableDictionary dictionary] retain];
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self initUIComponents];
    [self updateUIComponents];
    // Do any additional setup after loading the view from its nib.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)dealloc{
    [_documentDir release];
    [_prefixDir release];
    [_filename release];
    [_renderedImgPath release];
    [_reflowImgPath release];
    [_tesseractLang release];
    [_tesseractOCRText release];
    [_reflowImgView release];
    [_rotationAngle release];
    [_splitterOCRDict release];
    [super dealloc];
}

#pragma mark - Getter & Setter
- (void)setOption:(PDFParserOption)option{
    if(_option == option)
        return;
    _option = option;
    [self updateUIComponents];
}

- (void)setCurPage:(NSInteger)curPage{
    if(curPage <1 || curPage>self.pdfPageCount || _curPage==curPage)
        return;
    _curPage = curPage;
    [self clearData];
    [self createFolderIfNotExists];
    [self updateUIComponents];
}

- (void)setReflowImgPath:(NSString *)reflowImgPath{
    if(_reflowImgPath == reflowImgPath)
        return;
    [_reflowImgPath release];
    _reflowImgPath = [reflowImgPath retain];
    if(_reflowImgPath.length){
        UIImage *reflowImage = [UIImage imageWithContentsOfFile:self.reflowImgPath];
        CGSize size = reflowImage.size;
        self.reflowImgView.frame = CGRectMake(0, 0, size.width, size.height);
        self.reflowImgView.image = reflowImage;
        self.reflowImgView.center = self.reflowImgView.center;
        self.reflowScrollView.contentSize=size;
    }
    else{
        [self.reflowImgView setImage:nil];
    }
}

- (void)setRenderedImgPath:(NSString *)renderedImgPath{
    if(_renderedImgPath == renderedImgPath)
        return;
    [_renderedImgPath release];
    _renderedImgPath = [renderedImgPath retain];
    if(_renderedImgPath.length){
        self.mupdfImgView.image=[UIImage imageWithContentsOfFile:_renderedImgPath];
    }
    else
        [self.mupdfImgView setImage:nil];
}

- (void)setTesseractOCRText:(NSString *)tesseractOCRText{
    if(_tesseractOCRText == tesseractOCRText)
        return;
    [_tesseractOCRText release];
    _tesseractOCRText = [tesseractOCRText retain];
    if(_tesseractOCRText.length){
        [self.ocrTextView setText:_tesseractOCRText];
        [self.ocrTextView scrollRangeToVisible:NSMakeRange(_tesseractOCRText.length, 0)];
        [self.ocrTextView setScrollEnabled:NO];
        [self.ocrTextView setScrollEnabled:YES];
    }
    else{
        [self.ocrTextView setText:@""];
    }
}

- (void)setRotationAngle:(NSNumber *)rotationAngleValue{
    CGFloat angle = rotationAngleValue.floatValue;
    if(angle<0 || angle>1 || _rotationAngle.floatValue == angle)
        return;
    [_rotationAngle release];
    _rotationAngle =[[NSNumber numberWithFloat:360*angle] retain];
    [self clearData];
    [self createFolderIfNotExists];
    [self updateUIComponents];
}

#pragma mark - Action
- (IBAction)onPdfParserOptionChanged:(id)sender {
    UISegmentedControl *segCtrl = sender;
    [self setOption:segCtrl.selectedSegmentIndex];
}

- (IBAction)onEditBarItemPressed:(id)sender{
    [self showPdfOptionsView:self.pdfOptionsView.hidden];
}

- (IBAction)onSelectPdfButtonPressed:(id)sender {
    NSArray *directoryContents=[[NSFileManager defaultManager] contentsOfDirectoryAtPath:self.documentDir error:nil];
    NSArray * onlyPdfs = [directoryContents filteredArrayUsingPredicate:
                          [NSPredicate predicateWithFormat:@"self ENDSWITH[cd] 'pdf'"]];
    UIActionSheet* sheet = [[UIActionSheet alloc] init];
    sheet.title = @"选取Pdf";
    sheet.delegate = self;
    for(NSString *fileName in onlyPdfs)
        [sheet addButtonWithTitle:fileName];
    sheet.cancelButtonIndex = [sheet addButtonWithTitle:@"Cancel"];
    [sheet showInView:self.view];
    sheet.tag = kAlertViewPdfTag;
    [sheet release];
}

- (IBAction)onSelectLangButtonPressed:(id)sender {
    NSArray *directoryContents=[[NSFileManager defaultManager] contentsOfDirectoryAtPath:[self.documentDir stringByAppendingString:@"/tessdata"] error:nil];
    NSArray * onlyOCRLangs = [directoryContents filteredArrayUsingPredicate:
                          [NSPredicate predicateWithFormat:@"self ENDSWITH[cd] 'traineddata'"]];
    UIActionSheet* sheet = [[UIActionSheet alloc] init];
    sheet.title = @"选取OCR语言";
    sheet.delegate = self;
    for(NSString *fileName in onlyOCRLangs)
        [sheet addButtonWithTitle:fileName];
    sheet.cancelButtonIndex = [sheet addButtonWithTitle:@"Cancel"];
    [sheet showInView:self.view];
    sheet.tag = kAlertViewLangTag;
    [sheet release];
}

- (IBAction)onChangePageButtonPressed:(id)sender {
    UIButton *aButton = sender;
    [self setCurPage:self.curPage+(aButton.tag?1:-1)];
}

- (IBAction)onZoomLevelStepperChanged:(id)sender {
    UIStepper *stepper = sender;
    self.zoomlevel = 100*(stepper.value*stepper.value);
    [self clearData];
    [self createFolderIfNotExists];
    [self updateUIComponents];
}

- (IBAction)onRotationAngleSliderChanged:(id)sender {
    static CGFloat angle = 0;
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(setRotationAngle:) object:@(angle)];
    UISlider *slider = sender;
    angle = slider.value;
    [self performSelector:@selector(setRotationAngle:) withObject:@(angle) afterDelay:.5f];
}

- (IBAction)onReflowHorizStateValueChanged:(id)sender {
    [self setReflowImgPath:nil];
    [self updateUIComponents];
}

#pragma mark - Gesture Recognizer
- (void)handleSingleTap:(UIGestureRecognizer *)aRecognizer{
    UIView *tappedView = aRecognizer.view;
    if([tappedView isEqual:self.pdfOptionMaskView]){
        [self onEditBarItemPressed:nil];
    }
}

#pragma mark - UITableView Datasource
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{
    return self.wordFragment_count;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath{
    return 72;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    static NSString *reuseIdentifier = @"WordFragmentOCRCell";
    NSInteger row = [indexPath row];
    WordFragmentOCRCell *cell = [tableView dequeueReusableCellWithIdentifier:reuseIdentifier];
    if(!cell){
        NSArray *array = [[NSBundle mainBundle] loadNibNamed:@"WordFragmentOCRCell" owner:nil options:nil];
        cell = [array objectAtIndex:0];
    }
    NSString *wordFragmentPath = [NSString stringWithFormat:@"%@/WordFragment%05d.png",self.prefixDir,row];
    UIImage *wordFragmentImg = [UIImage imageWithContentsOfFile:wordFragmentPath];
    [cell loadWithImage:wordFragmentImg ocrText:[self.splitterOCRDict objectForKey:wordFragmentPath]];
    [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
    [cell setDelegate:self];
    return cell;
}

#pragma mark - WordFragmentOCRCell Delegate
- (void)recognizeImage:(UIImage *)aImage ofCell:(WordFragmentOCRCell *)aCell{
    if(!self.tesseractLang.length){
        [CustomIndicator showIndicatorOnTimerWithType:Str andString:@"Choose a language!"];
        return;
    }
    NSInteger row = [[self.splitterTableView indexPathForCell:aCell] row];
    dispatch_async(self.syncQueue, ^{
        NSMutableString *ocrText = [NSMutableString stringWithString:@""];
        Tesseract* tesseract = [[Tesseract alloc] initWithDataPath:@"tessdata" language:self.tesseractLang];
        [tesseract setImage:aImage];
        [tesseract recognize];
        NSString *recognizedText = [tesseract recognizedText];
        [ocrText appendString:[recognizedText stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
        dispatch_async(dispatch_get_main_queue(), ^{
            NSString *wordFragmentPath = [NSString stringWithFormat:@"%@/WordFragment%05d.png",self.prefixDir,row];
            [self.splitterOCRDict setObject:ocrText forKey:wordFragmentPath];
            [self.splitterTableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:row inSection:0]] withRowAnimation:UITableViewRowAnimationAutomatic];
        });
        [tesseract clear];
    });
}

#pragma mark - UIActionSheet Delegate
-(void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex{
    if(buttonIndex==[actionSheet cancelButtonIndex])
        return;
    switch (actionSheet.tag) {
        case kAlertViewPdfTag:{
            self.filename=[NSString stringWithString:[actionSheet buttonTitleAtIndex:buttonIndex]];
            NSURL *url=[NSURL fileURLWithPath:[NSString stringWithFormat:@"%@/%@",self.documentDir,self.filename]];
            CGPDFDocumentRef docRef = CGPDFDocumentCreateWithURL((CFURLRef)url);
            self.pdfPageCount = CGPDFDocumentGetNumberOfPages(docRef);
            self.curPage=1;
            [self clearData];
            break;
        }
        case kAlertViewLangTag:{
            NSString *trainDataFileName = [NSString stringWithString:[actionSheet buttonTitleAtIndex:buttonIndex]];
            self.tesseractLang=[trainDataFileName substringToIndex:trainDataFileName.length-@".traineddata".length];
            break;
        }
        default:
            break;
    }
    [self onEditBarItemPressed:nil];
    [self createFolderIfNotExists];
    [self updateUIComponents];
}

#pragma mark - UIScrollView Delegate
#pragma mark - View Operations
-(void)renderPage{
    if(self.filename==nil){
        [CustomIndicator showIndicatorOnTimerWithType:Str andString:@"Select a pdf first!"];
        return;
    }
    switch (self.option) {
        case PDFParserOptionPdf:{
            NSURL *url=[NSURL fileURLWithPath:[NSString stringWithFormat:@"%@/%@",self.documentDir,self.filename]];
            UIImage *img = [UIImage imageWithPDFURL:url atSize:CGSizeMake(PdfReflowSourceImageWidth, PdfReflowSourceImageHeight) atPage:self.curPage];
            self.previewImgView.image = img;
        }
            break;
        case PDFParserOptionImage:{
            if(!self.renderedImgPath.length){
                [self.loadingView startAnimating];
                dispatch_async(self.syncQueue, ^{
                    NSString *renderImgPath = [self.prefixDir stringByAppendingString:@"/render.png"];
                    render([[NSString stringWithFormat:@"%@/%@",self.documentDir,self.filename] UTF8String],[renderImgPath UTF8String],self.curPage,self.zoomlevel,self.rotationAngle.floatValue);
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [self setRenderedImgPath:[NSString stringWithString:renderImgPath]];
                        [self.loadingView stopAnimating];
                    });
                });
            }
            else
                [self setRenderedImgPath:[NSString stringWithString:self.renderedImgPath]];
        }
        break;
        case PDFParserOptionReflow:{
            if(!self.renderedImgPath.length){
                [CustomIndicator showIndicatorOnTimerWithType:Str andString:@"Convert Pdf to image First!"];
                break;
            }
            if(!self.reflowImgPath.length){
                [self.loadingView startAnimating];
                dispatch_async(self.syncQueue, ^{
                    int width,height,timecost;
                    NSString *renderedImgPath = [self.prefixDir stringByAppendingString:@"/render.png"];
                    NSString *reflowImgPath = [self.prefixDir stringByAppendingString:@"/reflow.png"];
                    CGSize size = (self.reflowHorizSwitch.isOn?CGSizeMake(self.reflowScrollView.bounds.size.height, self.reflowScrollView.bounds.size.width):CGSizeMake(self.reflowScrollView.bounds.size.width, self.reflowScrollView.bounds.size.height));
                    [self setWordFragment_count:0];
                    doPdfReflow([renderedImgPath UTF8String],[reflowImgPath UTF8String], size.width,size.height, &width, &height, &timecost,&_wordFragment_count);
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [self setReflowImgPath:[NSString stringWithString:reflowImgPath]];
                        [self.loadingView stopAnimating];
                    });
                });
            }
            else
                [self setReflowImgPath:[NSString stringWithString:self.reflowImgPath]];
        }
            break;
        case PDFParserOptionOCR:{
            if(!self.renderedImgPath.length){
                [CustomIndicator showIndicatorOnTimerWithType:Str andString:@"Convert pdf to image First!"];
                break;
            }
            if(!self.reflowImgPath.length){
                [CustomIndicator showIndicatorOnTimerWithType:Str andString:@"Reflow image First!"];
                break;
            }
            if(!self.tesseractLang.length){
                [CustomIndicator showIndicatorOnTimerWithType:Str andString:@"Choose a language!"];
                break;
            }
            if(!self.tesseractOCRText.length){
                [self.loadingView startAnimating];
                dispatch_async(self.syncQueue, ^{
                    NSMutableString *ocrText = [NSMutableString stringWithString:@""];
                    Tesseract* tesseract = [[Tesseract alloc] initWithDataPath:@"tessdata" language:self.tesseractLang];
                    [tesseract setImage:[UIImage imageWithContentsOfFile:self.reflowImgPath]];
                    [tesseract recognize];
                    NSString *recognizedText = [tesseract recognizedText];
                    [ocrText appendString:[recognizedText stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [self setTesseractOCRText:[NSString stringWithString:ocrText]];
                        [self.loadingView stopAnimating];
                    });
                    [tesseract clear];
                });
            }
            else
                [self setTesseractOCRText:[NSString stringWithString:self.tesseractOCRText]];
        }
            break;
        default:
            break;
    }
}

- (void)initUIComponents{
    self.previewImgView.layer.borderColor = [UIColor lightGrayColor].CGColor;
    self.previewImgView.layer.borderWidth = 1.f;
    
    self.mupdfImgView.layer.borderColor = [UIColor blueColor].CGColor;
    self.mupdfImgView.layer.borderWidth = 1.f;
    
    self.reflowScrollView.layer.borderColor = [UIColor greenColor].CGColor;
    self.reflowScrollView.layer.borderWidth = 1.f;
    
    self.ocrTextView.layer.borderColor = [UIColor whiteColor].CGColor;
    self.ocrTextView.layer.borderWidth = 1.f;
    [self.reflowScrollView addSubview:self.reflowImgView];
    
    [self.splitterTableView setSeparatorStyle:UITableViewCellSeparatorStyleNone];
    UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap:)];
    tapRecognizer.numberOfTouchesRequired = 1;
    tapRecognizer.numberOfTapsRequired = 1;
    [self.pdfOptionMaskView addGestureRecognizer:tapRecognizer];
    [tapRecognizer release];
}

- (void)updateUIComponents{
    switch (self.option) {
        case PDFParserOptionPdf:
        {
            [self.previewImgView setHidden:NO];
            [self.mupdfImgView setHidden:YES];
            [self.reflowScrollView setHidden:YES];
            [self.splitterTableView setHidden:YES];
            [self.ocrTextView setHidden:YES];
        }
            break;
        case PDFParserOptionImage:{
            [self.previewImgView setHidden:YES];
            [self.mupdfImgView setHidden:NO];
            [self.reflowScrollView setHidden:YES];
            [self.splitterTableView setHidden:YES];
            [self.ocrTextView setHidden:YES];
        }
            break;
        case PDFParserOptionReflow:
        {
            [self.previewImgView setHidden:YES];
            [self.mupdfImgView setHidden:YES];
            [self.reflowScrollView setHidden:NO];
            [self.splitterTableView setHidden:YES];
            [self.ocrTextView setHidden:YES];
        }
            break;
        case PDFParserOptionSplitter:
        {
            [self.previewImgView setHidden:YES];
            [self.mupdfImgView setHidden:YES];
            [self.reflowScrollView setHidden:YES];
            [self.splitterTableView setHidden:NO];
            [self.splitterTableView reloadData];
            [self.ocrTextView setHidden:YES];
        }
            break;
        case PDFParserOptionOCR:{
            [self.previewImgView setHidden:YES];
            [self.mupdfImgView setHidden:YES];
            [self.reflowScrollView setHidden:YES];
            [self.splitterTableView setHidden:YES];
            [self.ocrTextView setHidden:NO];
        }
            break;
        default:
            break;
    }
    [self.pdfDescLabel setText:[NSString stringWithFormat:@"f%@-p%i-z%.2f-z%.f%@",(self.filename.length?self.filename:@""),self.curPage,self.zoomlevel,self.rotationAngle.floatValue,(self.tesseractLang.length?[@"-l" stringByAppendingString:self.tesseractLang]:@"")]];
    [self renderPage];
}

- (void)showPdfOptionsView:(BOOL)isShown{
    if(self.isInAnimation)
        return;
    if(isShown){
        self.isInAnimation = YES;
        [self.pdfOptionsView setHidden:NO];
        self.pdfOptionsView.frame = CGRectMake(0, self.view.bounds.size.height, self.pdfOptionsView.bounds.size.width, self.pdfOptionsView.bounds.size.height);
        self.pdfOptionsView.alpha=0.f;
        [UIView animateWithDuration:kAnimationDuration animations:^{
            self.pdfOptionsView.frame = CGRectMake(0, self.view.bounds.size.height-self.pdfOptionsView.bounds.size.height, self.pdfOptionsView.bounds.size.width, self.pdfOptionsView.bounds.size.height);
            self.pdfOptionsView.alpha=1.f;
        } completion:^(BOOL finished){
            self.isInAnimation = NO;
        }];
    }
    else{
        self.isInAnimation = YES;
        self.pdfOptionsView.frame = CGRectMake(0, self.view.bounds.size.height-self.pdfOptionsView.bounds.size.height, self.pdfOptionsView.bounds.size.width, self.pdfOptionsView.bounds.size.height);
        [UIView animateWithDuration:kAnimationDuration animations:^{
            self.pdfOptionsView.frame = CGRectMake(0, self.view.bounds.size.height, self.pdfOptionsView.bounds.size.width, self.pdfOptionsView.bounds.size.height);
            self.pdfOptionsView.alpha=0.f;
        } completion:^(BOOL finished) {
            [self.pdfOptionsView setHidden:YES];
            self.isInAnimation = NO;
        }];
    }
}

#pragma mark - Private Functions
- (void)createFolderIfNotExists{
    if(!self.filename.length)
        return;
    NSString *fileprefix = [NSString stringWithString:self.filename];
    NSInteger pos = [self.filename rangeOfString:@"."].location;
    if(pos != NSNotFound){
        fileprefix = [self.filename substringToIndex:pos];
    }
    self.prefixDir = [[NSString stringWithFormat:@"%@/%@-p%i-z%.f-r%.f",self.documentDir,fileprefix,self.curPage,self.zoomlevel,self.rotationAngle.floatValue] retain];
    sprintf(gAppFolder, "%s",[self.prefixDir UTF8String]);
    NSFileManager *defaultManager = [NSFileManager defaultManager];
    BOOL isDir;
    if (![defaultManager fileExistsAtPath:self.prefixDir isDirectory:&isDir]) {
        NSError *error;
        [defaultManager createDirectoryAtPath:self.prefixDir withIntermediateDirectories:YES attributes:nil error:&error];
    }
}

- (void)clearData{
    [self setRenderedImgPath:nil];
    [self setReflowImgPath:nil];
    [self setTesseractOCRText:nil];
    [self.splitterOCRDict removeAllObjects];
}
@end
