//
//  CustomIndicator.h
//  PRIS
//
//  Created by lvbingru on 12/6/12.
//
//  等待框和提示框的通用控件

#import <Foundation/Foundation.h>

// 提示类别
typedef enum {
    Default = 0,        // 只是转圈圈
    BookPreview,        // 书籍翻到最后一页
    FavoriteMark,       // 收藏成功
    FavoriteCancel,     // 收藏失败
    CommentSuccess,     // 评论成功
    ShareSuccess,       // 分享成功
    SaveSuccess,        // 保存一张图片成功
    SaveAllSuccess,     // 保存所有图片成功
    CustomIndicatorStatusNoNetwork,
    Str,                // 只是文字
    
    // 注：如果有新的类型，加在NumOfStatus上面
    NumOfStatus
}CustomIndicatorStatus;

#pragma mark CustomIndicator
@interface CustomIndicator : NSObject

// 初始化和销毁，分别在程序开始和结束时调用
+ (void)initGlobalInstance;
+ (void)uninitGlobalInstance;

// 显示模态的等待框
+ (void)showLoadingView;
// 隐藏等待转圈
+ (void)hideLoadingView;
// 显示各种提示框
+ (void)showIndicatorOnTimerWithType:(CustomIndicatorStatus)aType andString:(NSString *)aString;

+ (void)showIndicatorOnTimerString:(NSString *)aString size:(CGSize) aSize;

+ (void)showIndicatorOnTimerString:(NSString *)aString size:(CGSize) aSize centerY:(NSInteger) aCenterY;

// 显示非模态的等待框
+ (void)showLoadingViewNotModel;

//4.5.0 booklive新加
//设置旋转
+ (void)setCustomIndicatorRotate:(CGFloat)rotate;

//重置
+ (void)resetCustomIndicatorOrientation;

@end



