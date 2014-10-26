//
//  CustomIndicator.m
//  PRIS
//
//  Created by lvbingru on 12/6/12.
//
//

#import "CustomIndicator.h"
#import <QuartzCore/QuartzCore.h>

#define MARGINWIDTH  (15)
#define MARGINHEIGHT (15)

static CustomIndicator *gCustomIndicator = nil; // CustomIndicator的单例

@interface CustomIndicator() {
    CGFloat _directionAngle;    // 旋转方向的角度

    CGFloat _defaultSize;       // 框框默认大小（转圈圈时的框框边长）
    CGFloat _largeSize;         // 框框有文字图片时候的大小
    CGFloat _hugeSize;          // 大一点的框框大小
    
    NSString* _picNames[NumOfStatus];   //不同类型对应图片名字的数组
}

@property (nonatomic, retain) UIWindow *window;     // 包含框框的window

@property (nonatomic, assign) IBOutlet UIActivityIndicatorView *activityIndicator; // 转圈圈
@property (nonatomic, assign) IBOutlet UILabel *titleLabel;     // 文字
@property (nonatomic, assign) IBOutlet UIImageView *imageView;  // 图片
@property (nonatomic, assign) IBOutlet UIView *mainView;        // 框框（包含以上控件的view）

@property (nonatomic, assign) CGSize customSize;
@property (nonatomic, assign) NSInteger customCenterY;
@end

@implementation CustomIndicator

#pragma mark - 单例创建 & 销毁
+ (void)initGlobalInstance
{
    if (gCustomIndicator == nil)
	{
		gCustomIndicator = [[CustomIndicator alloc] init];
	}
}

+ (void)uninitGlobalInstance
{
    [gCustomIndicator release];
    gCustomIndicator = nil;
}

#pragma mark - 显示&隐藏的公用接口
+ (void)showLoadingView
{
    [CustomIndicator initGlobalInstance];
    [gCustomIndicator showWaitingDialogOnString:nil withType:Default];
}

+ (void)showLoadingViewNotModel
{
    [CustomIndicator initGlobalInstance];
    [gCustomIndicator showWaitingDialogOnString:nil withType:Default];
    [gCustomIndicator.window setUserInteractionEnabled:NO];
}

+ (void)hideLoadingView
{
    [CustomIndicator initGlobalInstance];
    [gCustomIndicator hideWaitingDialog];
}

+ (void)showIndicatorOnTimerWithType:(CustomIndicatorStatus)aType andString:(NSString *)aString
{
    [CustomIndicator initGlobalInstance];
    [gCustomIndicator showWaitingDialogOnString:aString withType:aType];
}

//设置旋转
+ (void)setCustomIndicatorRotate:(CGFloat)rotate
{
   
    
    [CustomIndicator initGlobalInstance];
    
    // 创建window  //4.5.0 booklive 改动
    CGRect screenBounds =[[UIScreen mainScreen] bounds];
    CGFloat maxSize = MAX(screenBounds.size.width, screenBounds.size.height);
    gCustomIndicator.window.frame =CGRectMake(screenBounds.origin.x, screenBounds.origin.y, maxSize, maxSize);
    gCustomIndicator.window.center = CGPointMake(screenBounds.size.width/2, screenBounds.size.height/2);
    
    gCustomIndicator.window.transform = CGAffineTransformMakeRotation(rotate);
}


//重置
+ (void)resetCustomIndicatorOrientation
{
    [CustomIndicator initGlobalInstance];
    gCustomIndicator.window.transform = CGAffineTransformIdentity;
    
    gCustomIndicator.window.frame = [[UIScreen mainScreen] bounds];
}


+ (void)showIndicatorOnTimerString:(NSString *)aString size:(CGSize) aSize
{
    [CustomIndicator initGlobalInstance];
    gCustomIndicator.customSize = aSize;
    [gCustomIndicator showWaitingDialogOnString:aString withType:Str];
    gCustomIndicator.customSize = CGSizeZero;
}

+ (void)showIndicatorOnTimerString:(NSString *)aString size:(CGSize) aSize centerY:(NSInteger) aCenterY
{
    [CustomIndicator initGlobalInstance];
    gCustomIndicator.customSize = aSize;
    gCustomIndicator.customCenterY = aCenterY;
    [gCustomIndicator showWaitingDialogOnString:aString withType:Str];
    gCustomIndicator.customSize = CGSizeZero;
    gCustomIndicator.customCenterY = 0;
}

#pragma mark - 私有函数
#pragma mark 内存释放
- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [_window release];
    
	[super dealloc];
}

#pragma mark 初始化
- (id)init
{
	self = [super init];
	if (self)
	{
        /*
        // 创建window  //4.5.0 booklive 改动
        CGRect screenBounds =[[UIScreen mainScreen] bounds];
        CGFloat maxSize = MAX(screenBounds.size.width, screenBounds.size.height);
        UIWindow *window= [[UIWindow alloc] initWithFrame:CGRectMake(screenBounds.origin.x, screenBounds.origin.y, maxSize, maxSize)];
        //设置中心点
        window.center = CGPointMake(screenBounds.size.width/2, screenBounds.size.height/2);
        */
        
        UIWindow *window= [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
        [window setWindowLevel:UIWindowLevelStatusBar + 3];
        [window setUserInteractionEnabled:NO];
        [self setWindow:window];
        [window release];

        // 加载界面
        [[NSBundle mainBundle] loadNibNamed:@"CustomIndicator" owner:self options:nil];
        [self initViews:self.mainView labelView:self.titleLabel imageView:self.imageView];
        [self.window addSubview:self.mainView];
        [self.mainView.layer setCornerRadius:7.0];
        [self relocateView];

        
        // 监听界面旋转通知
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(statusBarDidChangeNotifictation:) name:UIApplicationDidChangeStatusBarOrientationNotification object:nil];
        
        // 初始化数据（pad和phone的各自图片设置）
        _picNames[Default] = nil;
        _picNames[Str] = nil;
        _picNames[BookPreview] = @"icon_endpage";
        _picNames[FavoriteMark] = @"public_shoucang_highLight_normal";
        _picNames[FavoriteCancel] = @"public_shoucang_normalMode";
        _picNames[CommentSuccess] = @"public_pinglun_normalMode";
        _picNames[ShareSuccess] = @"picture_share";
        _picNames[SaveSuccess] = @"picture_download";
        _picNames[SaveAllSuccess] = @"picture_download";
        _picNames[CustomIndicatorStatusNoNetwork] = @"public_none";
        
        _defaultSize = 101.0f;
        _largeSize = 101.0f;
        _hugeSize = 182.0f;
	}
	return self;
}

#pragma mark 显示&隐藏的私用接口
- (void)hideWaitingDialog
{
    [self.mainView.layer removeAllAnimations];
    [self.activityIndicator stopAnimating];
    [self.window setUserInteractionEnabled:NO];
    [self.window setHidden:YES];
}

- (void)showWaitingDialog
{
    [self.mainView.layer removeAllAnimations];
    [self.window setHidden:NO];
    [self.activityIndicator startAnimating];
}

- (void)showWaitingDialogOnString:(NSString *)aString withType:(CustomIndicatorStatus)aType
{
    [self.mainView.layer removeAllAnimations];
    [self relocateView];
    [self.window setHidden:YES];
    
    [self.titleLabel setText:aString];
    
    if(aType == Default)
    {
        [self.imageView setImage:nil];
        [self resizeWithSize:_defaultSize];
        [self.window setHidden:NO];
        [self.window setUserInteractionEnabled:YES];
        [self.activityIndicator startAnimating];
    }
    else
    {
        BOOL isPositiveFeedBack = YES;
        
        [self.activityIndicator stopAnimating];
        
       
        
        //初始化图片
        [self.imageView setImage:[UIImage imageNamed:_picNames[aType]]];
        
        //设置bounds大小
        [self resizeViewBoundsByLabel:self.titleLabel byString:aString byImage:self.imageView];
        //设置frame位置
        [self setMainViewFrame:self.mainView labelView:self.titleLabel imageView:self.imageView];

        if(aType == FavoriteCancel)
        {
            isPositiveFeedBack = NO;
        }
        
        // 初始缩放大小
        CGFloat scale0 = isPositiveFeedBack ? 2.0 : 0.001;
        [self makeScale:scale0];
        [self.mainView setAlpha:0.0];
        [self.window setHidden:NO];
        
        // 动画1：缩放大小从scale0到1.0
        CGFloat scale1 = 2.001 - scale0;
        [UIView animateWithDuration:0.3 animations:^(void) {
            [self makeScale:1.0];
            [self.mainView setAlpha:1.0];
        } completion:^(BOOL finished) {
            // 动画2: 缩放大小1.0到scale1
            [UIView animateWithDuration:0.3 delay:0.8 options:UIViewAnimationCurveEaseInOut animations:^(void) {
                [self makeScale:scale1];
                [self.mainView setAlpha:0.0];
            } completion:^(BOOL finished) {
                // 动画结束，隐藏window
                [self.window setHidden:YES];
                [self makeScale:1.0];
                [self.mainView setAlpha:1.0];
                [self.imageView setImage:nil];
            }];
        }];
    }
}

- (void)initViews: (UIView *)aMainView labelView:(UILabel *)aLabel imageView: (UIImageView *)aImageView
{
    /*------------------init label-----------------*/
    //设置自动行数与字符换行
    [aLabel setNumberOfLines:0];
    aLabel.lineBreakMode = UILineBreakModeWordWrap;
    
    //居中显示
    aLabel.textAlignment = UITextAlignmentCenter;
}

#pragma ViewBounds
//根据字符串、图片来调整显示框的大小
- (void)resizeViewBoundsByLabel: (UILabel *)aLabel byString: (NSString*) aStr byImage: (UIImageView *)aImageView
{
    /*------------------init label  bounds-----------------*/
    CGRect labelRect;
    labelRect.origin = CGPointZero;
    labelRect.size = CGSizeZero;
    
    //有字符串的情况
    if (aStr != nil && (![aStr isEqualToString:@""]))
    {
        UIFont *font = aLabel.font;
        
        //设置一个行高上限
        CGSize size = CGSizeMake(250,1000);
        
        //计算实际frame大小，并将label的frame变成实际大小
        CGSize labelsize = [aStr sizeWithFont:font constrainedToSize:size lineBreakMode:UILineBreakModeWordWrap];
        
        
        labelRect = CGRectMake(0,0, ceilf(labelsize.width), ceilf(labelsize.height));
        
        [aLabel setBounds:labelRect];
    }
    
    /*------------------init imageView  bounds-----------------*/
    CGRect imageViewRect;
    imageViewRect.origin = CGPointZero;
    imageViewRect.size = CGSizeZero;
    
    //有图片存在
    if (aImageView.image != nil)
    {
        imageViewRect.size.width = ceilf(aImageView.image.size.width);
        imageViewRect.size.height = ceilf(aImageView.image.size.height + MARGINHEIGHT * 1.5);
    }
    [aImageView setBounds:imageViewRect];
    
    /*------------------init mainView  bounds-----------------*/
    CGSize mainViewSize;
    mainViewSize.width =  (imageViewRect.size.width > labelRect.size.width) ? imageViewRect.size.width : labelRect.size.width;
    mainViewSize.width += (MARGINWIDTH * 2);
    
    mainViewSize.height = labelRect.size.height + imageViewRect.size.height + (MARGINHEIGHT * 2);
    CGRect bounds = CGRectMake(0,0, ceil(mainViewSize.width), ceil(mainViewSize.height));
    
    //设置bounds的最大、最小宽度， 最小高度
    bounds = [self limitMainBoundsSize:bounds withImage:((aImageView.image != nil) ? YES : NO)];
    
    [self.mainView setBounds:bounds];
    
}

- (CGRect)limitMainBoundsSize:(CGRect) bounds withImage:(BOOL *)aImage
{
    if (bounds.size.width < 100)
    {     //最小宽度为100
        bounds.size.width = 100;
    }
    else if(bounds.size.width > 280)
    {     //最小宽度为280
        bounds.size.width = 280;
    }
    
    //最小高度为50
    if (aImage == NO)
    {   //只有文字的时候， 高度最小为50
        if (bounds.size.height < 50)
        {
            bounds.size.height = 50;
        }
    }
    else
    {   //有图片，有文字，高度最小为100
        if (bounds.size.height < 100)
        {
            bounds.size.height = 100;
        }
    }
    return bounds;
}

//重置mainView、 label 和  imageView的 frame
- (void)setMainViewFrame:(UIView *)aMainView labelView:(UILabel *)aLabel imageView: (UIImageView *)aImageView
{
    CGRect mainViewBounds = aMainView.bounds;
    CGRect labelViewBounds = aLabel.bounds;
    CGRect imageViewBounds = aImageView.bounds;
    
    //init imageView frame
    CGPoint imageFramePoint;
    imageFramePoint.x = abs((mainViewBounds.size.width - imageViewBounds.size.width)/2);
    imageFramePoint.y = abs((mainViewBounds.size.height - (imageViewBounds.size.height + labelViewBounds.size.height) )/2);

    [aImageView setFrame:CGRectMake(imageFramePoint.x, imageFramePoint.y, imageViewBounds.size.width, imageViewBounds.size.height)];
    
    //init labelView frame
    CGPoint labelFramePoint;
    labelFramePoint.x = abs((mainViewBounds.size.width - labelViewBounds.size.width)/2);
    labelFramePoint.y = imageFramePoint.y + imageViewBounds.size.height;
    [aLabel setFrame:CGRectMake(labelFramePoint.x, labelFramePoint.y, labelViewBounds.size.width, labelViewBounds.size.height)];
    
}

#pragma mark 调整框框的位置和大小
- (void)relocateView
{
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    if (self.customCenterY > 0){
        [self.mainView setCenter:CGPointMake(self.window.bounds.size.width/2.0f, self.customCenterY)];
    }
    else{
        [self.mainView setCenter:CGPointMake(self.window.bounds.size.width/2.0f, self.window.bounds.size.height/2.0f)];
    }

    switch (orientation) {
        case UIInterfaceOrientationLandscapeLeft:
            _directionAngle = -M_PI/2.0;
            break;
        case UIInterfaceOrientationLandscapeRight:
            _directionAngle = M_PI/2.0;
            break;
        case UIInterfaceOrientationPortrait:
            _directionAngle = 0.0;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            _directionAngle = M_PI;
            break;
        default:
            break;
    }
    [self.mainView setTransform:CGAffineTransformMakeRotation(_directionAngle)];
}

- (void)makeScale:(CGFloat)aScale
{
    [self.mainView setTransform:CGAffineTransformConcat(CGAffineTransformMakeRotation(_directionAngle), CGAffineTransformMakeScale(aScale, aScale))];
}

- (void)resizeWithSize:(CGFloat)size
{
    [self.mainView setBounds:CGRectMake(0.0, 0.0, size, size)];
}

- (void)resizeWithSizeEx:(CGSize)size
{
    [self.mainView setBounds:CGRectMake(0.0, 0.0, size.width, size.height)];
}


#pragma mark 旋转通知
- (void)statusBarDidChangeNotifictation:(NSNotification *)notification
{
    [self relocateView];
}
@end