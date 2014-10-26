//
//  WordFragmentOCRCell.m
//  iPdfParser
//
//  Created by KyleWong on 14-10-6.
//  Copyright (c) 2014年 kylewong. All rights reserved.
//

#import "WordFragmentOCRCell.h"

@interface WordFragmentOCRCell()
@property(nonatomic,assign) IBOutlet UIImageView * wordFragmentImgView;
@property(nonatomic,assign) IBOutlet UILabel *ocrTextLabel;
@property(nonatomic,assign) IBOutlet UIButton *ocrButton;
@property(nonatomic,assign) IBOutlet UIActivityIndicatorView *loadingView;
@end

@implementation WordFragmentOCRCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        // Initialization code
    }
    return self;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

- (void)dealloc{
    _delegate = nil;
    [super dealloc];
}

#pragma mark - Action
- (IBAction)onOCRButtonPressed:(id)sender {
    if(self.delegate && [self.delegate respondsToSelector:@selector(recognizeImage:ofCell:)]){
        [self.loadingView startAnimating];
        [self.delegate recognizeImage:self.wordFragmentImgView.image ofCell:self];
    }
}

#pragma mark - Public Interfaces
- (void)loadWithImage:(UIImage *)aImage ocrText:(NSString *)aOcrText{
    [self.wordFragmentImgView setImage:aImage];
    [self.ocrTextLabel setText:aOcrText];
    [self updateUIComponents];
}

- (void)updateOCRText:(NSString *)aOCRText{
    [self.ocrTextLabel setText:aOCRText];
    [self updateUIComponents];
}

#pragma mark - Private Functions
- (void)updateUIComponents{
    if(self.ocrTextLabel.text.length){
        [self.ocrButton setBackgroundImage:nil forState:UIControlStateNormal];
        [self.ocrButton setTitle:@"->" forState:UIControlStateNormal];
        [self.ocrButton setEnabled:NO];
        [self.loadingView stopAnimating];
    }
    else{
        [self.ocrButton setBackgroundImage:[UIImage imageNamed:@"character"] forState:UIControlStateNormal];
        [self.ocrButton setTitle:nil forState:UIControlStateNormal];
        [self.ocrButton setEnabled:YES];
    }
}
@end
