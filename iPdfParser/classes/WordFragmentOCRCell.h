//
//  WordFragmentOCRCell.h
//  iPdfParser
//
//  Created by KyleWong on 14-10-6.
//  Copyright (c) 2014年 kylewong. All rights reserved.
//

#import <UIKit/UIKit.h>
@class WordFragmentOCRCell;

@protocol WordFragmentCellDelegate <NSObject>
- (void)recognizeImage:(UIImage*)aImage ofCell:(WordFragmentOCRCell *)aCell;
@end

@interface WordFragmentOCRCell : UITableViewCell
@property (nonatomic,assign) id<WordFragmentCellDelegate> delegate;
- (void)loadWithImage:(UIImage *)aImage ocrText:(NSString *)aOcrText;
- (void)updateOCRText:(NSString *)aOCRText;
@end
