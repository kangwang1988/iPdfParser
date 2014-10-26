##iPdfParser Release V1.0

-	此文档描述了iOS下的一个Pdf处理组件iPdfParser。阅读此文档，你将知道其原理如何，有哪些功能，以及如何去使用其中的部分特性。

-	什么是iPdfParser?

	iPdfParse是一个用于pdf处理的功能组件。你可以使用它完成pdf页面渲染(转化为png等图片)，重排页面以适应小屏幕下的显示，将页面切割成单个文字图片，以及图片的光学文字识别。
	其使用了一些著名的开源项目，包括leptonica、tesseract、k2pdfopt/willuslib、zlib、freetype、libpng、jbig2dec、fitz等，核心功能代码均使用C/C++实现，可以便捷地移植到其他平台下。所有的代码均以源码出现，如果你仅需要其部分特性，可能只需其部分代码。
	
-	如何编译安装?
	
	工程文件在Xcode5.1.1 iOS7.1 SDK下编译。不过由于除页面显示外并未使用其他Cocoa Touch的SDK，在你的编译环境下应该也能正常工作。
	

-	如何使用？
	
	对于开发者，你可能需要用到以下的函数调用:
	1. Pdf 页面渲染 ==> void render(char* infilename,char * outfilename, int pagenumber, int zoom, int rotation)
	此函数在pdf2image.c中实现,infileName代表需要渲染的pdf路径，outfilename指渲染所得图片,pagenumber指重排页面的页码(1代表第一页),zoom代表放大倍数(基准100，调用时如200等),rotation代表旋转角度.
	2. Pdf 页面重排 ==> void doPdfReflow(char *inPngPath,char *outPngPath,int devWidth,int devHeight,int *width,int *height,double *timeCost,int *wordFragmentCount)
	此函数在koptreflow.c中实现,inPngPath和outPngPath分别指代重排前后的图片路径,devWidth决定了重排图片的宽度,devHeight代表设备高度以避免重排时讲单个文字行分割到两个相邻页面,width和宽度代表了重排后图片的尺寸,timecost指重排耗费时间，wordFragment指重排时切割文字所得到的数目.
	3. Pdf页面切割 => doPdfReflow
	Pdf页面重排时即完成了页面切割，输出路径位于APP的documents目录下。
	4. 光学文本识别 =>  (void)recognizeImage:(UIImage *)aImage ofCell:(WordFragmentOCRCell *)aCell
	这里aCell和识别无关，aImage代表了需要识别的文本图片。
	将页面切割成单个文本并识别是一种分而治之的策略，尤其是当整张图片需要过长时间去识别时。
		
	对于用户，你可能需要知道:
	1. 设计中，首先Pdf页面渲染成图片，渲染所得图片进行重排，重排时切割页面，切割页面所得单字图片，单字图片进行识别，这些操作是由先后顺序的。
	2. 将需要处理的pdf和tessdata语言支持文件夹放到APP的Document路径下，并在文本识别前选择一个语言，文字识别依赖于这些数据。
	3. 点击页面顶部右侧Edit按钮可以选择Pdf(位于Documents目录下)、语言支持(位于Documents/tessdata/*.traindata)、页码切换、渲染放大倍数、旋转角度、重排时输出图片宽高设置，点击页面上方切换tab，可以完成相关的功能。
		
-	Q&A:

	Q. 缓存管理:
	A. 重排状态下阅读一个有很多页码的Pdf时，需要注意内存、磁盘使用。当不需要的时候记得释放这些数据。
	
	Q. 如何去改变重排字体大小?
	A. 将Pdf页面渲染成更大尺寸的页面，重排后可得到大号字体。
	
	Q. 重排时页面可否使用png以外的格式?
	A. 不需要，重排依赖于位图数据，即使是png也是首先读到了位图数据，然后再处理。如果需要其他图片格式，你需要导入一些其他格式的解析库。
	
	Q. 如何提高重排效率?
	A. 可以通过将作为中介的渲染页面所得图片磁盘文件(*.png)读写的fread函数重定向到读写内存数据.此外，由于重排、文本识别等较耗费CPU，你可能需要在后台进行这些处理并在结束时通知主线程。
	
- 更多
  
  [联系我们](mailto:kang.wang1988@gmail.com)
