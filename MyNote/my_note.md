[Toc]

# 第一篇 git 与github 的使用心得
## 1.1 基础
### 1.1.1 安装git
```bash
	sudo apt-get install git
```
### 1.1.2.在github上申请一个账号
  找个笔记本记住账号和密码
  从https://github.com/oaid/Tengine.git fork一个仓库。
  比如我fork 了一个仓库，url地址：https://github.com/RudyFoo/Tengine.git

### 1.1. 3.从我的github仓库下载代码
```bash
	git clone https://github.com/RudyFoo/Tengine.git
```
### 1.1.4 上传代码到github仓库
```bash
	git add file1.xxx file2.xxx #可以使用通配符*
	git commit -m "add 2 file"  #写此次提交的简略信息
	git push		    #推送到github仓库
```
### 1.1.4 分支操作
```bash
	git checkout -b dev 	#创建并切换分支
```
```bash
	git branch -a 		#查看分支
```
```bash
	git checkout dev 	#切换分支
```
### 1.1.5 操作git config
```bash
	git config --show-origin -l 		#查看
```
```bash
	git config --global --edit 		#编辑
```
## 1.2 进阶
### 1.2.0 最少git clone
```bash
	git clone --depth=1  https://github.com/RudyFoo/Tengine.git
```
### 1.2.1 添加主仓库分支
```bash
	git remote add -f upstream https://github.com/oaid/Tengine.git  # -f 意思是去fetch
	# git remote remove  upstream  					# 删除跟踪仓库
	git fetch upstream  				# 要是remote add 不加fetch，需要执行这条指令才能后续操作pull
	git remote -v 					#查看添加的源
```
### 1.2.2 同步主仓库分支

```bash
	git log origin/master -1 && git log upstream/master -1 		#查看两仓库最新日志
```

```bash
	git pull upstream  						#从主仓库下载最新代码
	git push 							#把从主仓库下载下来的代码推送到自己的github 仓库
```

### 1.2.3 [Git强制覆盖master分支](https://www.cnblogs.com/king-le0/p/10097583.html)
假如你有两个分支，develop和master，你想用前者替换后者，如下操作即可。
a. 切换到develop分支下，并保证本地已经同步了远端develop的最新代码。
```bash
	git  checkout develop  
	git  pull 
```

b. 把本地的develop分支强制(-f)推送到远端master。

```bash
	git  push origin develop:master -f 
```

c. 切换到旧分支master。

```bash
	git  checkout master 
```
d. 下载远程仓库最新内容，不做合并。
```bash
	git  fetch --all 
```
e. 把HEAD指向master最新版本。
```bash
	git  reset --hard origin/master 
```
### 1.2.4 删除分支
a.删除本地分支test_ci
```bash
	git branch -d test_ci
```
b.删除远程分支test_ci
```bash
	git push origin --delete test_ci
```
### 1.2.5 使用github分支提交pr
a.获取正式仓库的最新代码，放到本地的新建分支	

```bash
	git clone https://github.com/RudyFoo/Tengine.git #添加临时仓库
	cd Tengine
	git remote add -f upstream https://github.com/OAID/Tengine.git #添加正式仓库,且去fetch
	git checkout -b dev upstream/master #创建并拉取分支
	#git fetch upstream upstream/master:dev #从upstream拉取master放到新建的dev分支
	#git checkout dev #[来自](https://www.jianshu.com/p/2db19fc37ac80)
```

b.用对比工具将修改同步到新建分支
		beyond compare
c.推送新分支到临时仓库      

```bash
	git push origin dev
```

d.在GitHub的web上提交，请求时选择dev分支与正式仓库合并


# 第二篇 深度学习

## 2.1 基础知识

### 1 算法总结

 

| **算法**                                                     |                                                              |                                                              |                                             |      |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------- | ---- |
|                                                              | 类型                                                         | 名称                                                         | 说明                                        | 商业 |
| 人脸相关的                                                   | 参考博客： https://blog.csdn.net/qq_34654240/article/details/82860391 |                                                              |                                             |      |
| 人脸识别                                                     | mtcnn                                                        | 人脸识别                                                     | open ai lab                                 |      |
| insightface                                                  | 人脸识别 包含：RetinaFace，ArcFace                           | 微众银行                                                     |                                             |      |
| retinaface                                                   | 人脸检测，人脸对齐 模型 https://github.com/Charrin/RetinaFace-Cpp/tree/master/convert_models/mnet |                                                              |                                             |      |
| RetinaFace MobileNet0.25                                     | https://github.com/deepinsight/insightface/issues/669        |                                                              |                                             |      |
| Libfacedetection                                             | https://github.com/ShiqiYu/libfacedetection                  |                                                              |                                             |      |
| LFGFD：Ultra-Light-Fast-Generic-Face-Detector-1MB            | https://github.com/Linzaer/Ultra-Light-Fast-Generic-Face-Detector-1MB |                                                              |                                             |      |
| LFFD 6.1MA-Light-and-Fast-Face-Detector-for-Edge-Devices     | https://github.com/YonghaoHe/A-Light-and-Fast-Face-Detector-for-Edge-Devices |                                                              |                                             |      |
| CenterFace                                                   | https://github.com/Star-Clouds/CenterFace                    |                                                              |                                             |      |
| DBFace                                                       | https://github.com/dlunion/DBFace                            |                                                              |                                             |      |
| sphereFace                                                   |                                                              |                                                              |                                             |      |
|                                                              |                                                              | mxnet，我们有demo                                            |                                             |      |
|                                                              | PFID人脸关键点                                               | https://github.com/hanson-young/nniefacelib/blob/master/PFPLD/README.md  基于nnie的量化部署（还有mobilefacenet, retinafacenet）https://zhuanlan.zhihu.com/p/120376337 |                                             |      |
| 人脸检测                                                     | LFFD                                                         | 轻量级人脸检测 https://github.com/YonghaoHe/A-Light-and-Fast-Face-Detector-for-Edge-Devices |                                             |      |
|                                                              |                                                              |                                                              |                                             |      |
| **语义分割**                                                 | YOLACT++                                                     | 分割 https://zhuanlan.zhihu.com/p/97684893                   |                                             |      |
|                                                              | mask_rcnn_inception_resnet_v2_atrous_cocomask_rcnn_resnet101_atrous_cocomask_rcnn_resnet50_atrous_cocomask_rcnn_inception_v2_coco |                                                              |                                             |      |
| 语义分割是将输入图像中的每个像素分配一个语义类别，以得到像素化的密集分类 FCN、SegNet、U-Net、FC-Densenet E-Net 和 Link-Net、RefineNet、PSPNet、Mask-RCNN 参考： https://www.tinymind.cn/articles/410?from=articles_commend |                                                              |                                                              |                                             |      |
| FPN                                                          | 图像金字塔思想                                               |                                                              |                                             |      |
| SegNet                                                       | 语义分割                                                     |                                                              |                                             |      |
|                                                              |                                                              |                                                              |                                             |      |
| 目标检测                                                     | 分类网络                                                     | LeNet                                                        | 图片识别分类                                |      |
| AlexNet                                                      | ILSVRC-2012冠军                                              |                                                              |                                             |      |
| VGG16                                                        | ILSVRC-2014中定位任务第一名和分类任务第二名                  | 视听研究院：闸机，人体特征点，活体                           |                                             |      |
| GoogleNet                                                    | ILSVRC 2014 上取得了第一名的成绩                             |                                                              |                                             |      |
| Resnet18                                                     | ILSVRC-2015冠军                                              |                                                              |                                             |      |
| Resnet50                                                     | ILSVRC-2015冠军                                              |                                                              |                                             |      |
| SPPNet                                                       | ImageNet 2014的比赛中，此方法检测中第二，分类中第三 不用考虑图片大小，在图像变形的情况下表现稳定 |                                                              |                                             |      |
| DenseNet                                                     | 图像分类                                                     |                                                              |                                             |      |
| R-CNN                                                        | 目标检测                                                     |                                                              |                                             |      |
|                                                              | shufflenetv2                                                 | 分类网络                                                     |                                             |      |
| **检测网络**                                                 | Faster R-CNN                                                 |                                                              |                                             |      |
| YOLOV2                                                       |                                                              |                                                              |                                             |      |
| YOLOV3                                                       |                                                              |                                                              |                                             |      |
| SSD                                                          |                                                              |                                                              |                                             |      |
| RFCN                                                         |                                                              |                                                              |                                             |      |
| **识别检测**                                                 | PSPNet                                                       | 场景解析，FCN ，Pyramid Scene Parsing Network                |                                             |      |
| Mask R-CNN                                                   |                                                              |                                                              |                                             |      |
| SiameseNet                                                   |                                                              |                                                              |                                             |      |
| SqueezeNet                                                   |                                                              |                                                              |                                             |      |
| DCGAN                                                        |                                                              |                                                              |                                             |      |
| NIN                                                          |                                                              |                                                              |                                             |      |
| 语音                                                         | 语音识别                                                     | DeepSpeech                                                   |                                             |      |
| Wavenet                                                      |                                                              |                                                              |                                             |      |
| KWS                                                          |                                                              |                                                              |                                             |      |
|                                                              |                                                              | 图片检测                                                     | https://github.com/devzwy/open_nsfw_android |      |
|                                                              | OCR                                                          | chineseocr                                                   | https://github.com/chineseocr/chineseocr    |      |
|                                                              | tesseract                                                    | https://github.com/tesseract-ocr/tesseract                   |                                             |      |
|                                                              |                                                              | [OCRmyPDF](https://github.com/jbarlow83/OCRmyPDF)            | https://github.com/jbarlow83/OCRmyPDF       |      |

算法仓库： http://manaai.cn/index3.html  

[     https://aizoo.com/](https://aizoo.com/) 

​     极视角： http://www.cvmart.net/ 

华为云： https://www.huaweicloud.com/theme/241628-1-M  

   https://support.huaweicloud.com/qs-modelarts/modelarts_06_0002.html 

   

ModelArts自动学习能力，可根据用户标注数据全自动进行模型设计、参数调优、模型训练、模型压缩和模型部署全流程。无需任何代码编写和模型开发经验，即可利用ModelArts构建AI模型应用在实际业务中。

**分割：**

 

\1. mask_rcnn_inception_resnet_v2_atrous_coco

\2. mask_rcnn_resnet101_atrous_coco

\3. mask_rcnn_resnet50_atrous_coco

\4. mask_rcnn_inception_v2_coco

 **TensorFlow的检测模型：** 

**检测：** [  https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/detection_model_zoo.md](https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/detection_model_zoo.md) 

ssd_mobilenet_v1_coco  30  21  Boxes

ssd_mobilenet_v1_0.75_depth_coco ☆  26  18  Boxes

ssd_mobilenet_v1_quantized_coco ☆  29  18  Boxes

ssd_mobilenet_v1_0.75_depth_quantized_coco ☆  29  16  Boxes

ssd_mobilenet_v1_ppn_coco ☆  26  20  Boxes

ssd_mobilenet_v1_fpn_coco ☆  56  32  Boxes

ssd_resnet_50_fpn_coco ☆  76  35  Boxes

ssd_mobilenet_v2_coco  31  22  Boxes

ssd_mobilenet_v2_quantized_coco  29  22  Boxes

ssdlite_mobilenet_v2_coco  27  22  Boxes

ssd_inception_v2_coco  42  24  Boxes

faster_rcnn_inception_v2_coco  58  28  Boxes

faster_rcnn_resnet50_coco  89  30  Boxes

faster_rcnn_resnet50_lowproposals_coco  64    Boxes

rfcn_resnet101_coco  92  30  Boxes

faster_rcnn_resnet101_coco  106  32  Boxes

faster_rcnn_resnet101_lowproposals_coco  82    Boxes

faster_rcnn_inception_resnet_v2_atrous_coco  620  37  Boxes

faster_rcnn_inception_resnet_v2_atrous_lowproposals_coco  241    Boxes

faster_rcnn_nas  1833  43  Boxes

faster_rcnn_nas_lowproposals_coco  540    Boxes

mask_rcnn_inception_resnet_v2_atrous_coco  771  36  Masks

mask_rcnn_inception_v2_coco  79  25  Masks

mask_rcnn_resnet101_atrous_coco  470  33  Masks

mask_rcnn_resnet50_atrous_coco

| 客户             | 使用模型                                  | 格式                                | 用途场景 | 来源                                                         |
| ---------------- | ----------------------------------------- | ----------------------------------- | -------- | ------------------------------------------------------------ |
|                  |                                           |                                     |          | [ftp.eaidk.net/models/](http://ftp.eaidk.net/models/)user: ftpuserpasswd: ftpuser!@# |
| ABB              | vgg16                                     | Tensorflow                          |          | 客户                                                         |
| G7               | mobilenetV2ssdshufflenetv2mtcnn           | caffe/Tensorflow                    |          | 客户(caffe模型都是来自客户，mobilenetv2ssd tensorflow模型是客户给的开源地址下载) |
| 宇视             | shufflenetv2                              | caffe                               |          | 开源地址下载                                                 |
| 传音             | Densenetshufflenetv2                      | caffe/Tensorflow                    |          | densenet是咱们内部测试模型，shufflenetv2是根据宇视的开源地址得到 |
| 杭州智棱         | MTCNN                                     | caffe                               |          |                                                              |
| 科为升           | mobilenetV2ssdtinyyolov3                  | tensorflow                          |          | mobilenetV2ssd tensorflow模型同G7tinyyolov3来自客户          |
| 乐贤智能         | ssd-detection                             | tflite                              |          | 客户自己的模型                                               |
| 平安科技         | maskrcnn                                  | Tensorflow                          |          | 开源地址下载                                                 |
| 同温层           | yolov3                                    | 无                                  |          | 无                                                           |
| 新希望           | yolov3 maskrcnn                           | mxnet                               |          | 客户                                                         |
| 微众银行         | insightface( mobilefacenet ， retinaface) | mxnet/caffe mxnet/caffe mxnet/caffe |          | 开源收集，客户自己重训练                                     |
| 视听研究院       | vgg16                                     | TensorFlow                          |          |                                                              |
| 360              | mobilenet_V1                              |                                     |          |                                                              |
| 中电科城市研究院 | MobileNetSSD                              |                                     |          |                                                              |
| 安软惠视         | yolov3，centerNet                         |                                     |          |                                                              |
| 点泽科技         | yolov3                                    |                                     |          |                                                              |
| 苏州沃科汽车     | DMS 具体开源算法不清楚                    |                                     |          |                                                              |
| 帷幄科技         | landmark，SSD ，Libfacedetection          |                                     |          |                                                              |
| 致远电子         | mtcnn                                     |                                     |          |                                                              |
| 科道智能         | mobilenetV2                               |                                     |          |                                                              |
| 炼道科技         | yolov-tiny，YoLov3                        |                                     |          |                                                              |
| 迈达思敏         | adas ， dms 具体开源算法不清楚            |                                     |          |                                                              |
| 云析物联         | mtcnn                                     |                                     |          |                                                              |
| 中控智慧         | retinaface                                |                                     |          |                                                              |
| 视源股份         | retinaface                                |                                     |          |                                                              |

| 参考： https://blog.csdn.net/sparrow_jsl/article/details/83092569 |                                                              |      |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ---- |
| ILSVRC                                                       |                                                              |      |
| challenger.ai                                                | https://challenger.ai/ 创新工场，美团，美图等主办 ，涵盖了 短视频分析，无人驾驶，翻译等技术内容和相应的数据集 |      |
| 科赛/kesci                                                   | 开放的竞技平台                                               |      |
| SQuAD                                                        | 面向学生的竞技平台 https://rajpurkar.github.io/SQuAD-explorer/ |      |

| 数据集    |                                                       |                              |      |
| --------- | ----------------------------------------------------- | ---------------------------- | ---- |
| 名称      | 说明                                                  | 链接                         |      |
| LVIS      |                                                       | https://www.lvisdataset.org/ |      |
| MNIST     | 手写数字数据集 60000个训练样本                        |                              |      |
| GTSRB     | 德国交通标示数据集                                    |                              |      |
| RRSI      | 交通路网遥感图像数据集                                |                              |      |
| ImageNet  | 图片集，22000个类别的数据集，1500万副图像             |                              |      |
| CIFAR-10  | 常见物体图像数据集 ，10个类别，60000幅32*32的彩色图像 |                              |      |
| VOC2007   | 物体和场景的图像数据集                                |                              |      |
| Oxford-17 | 鲜花图像数据集 1360幅图像，17类                       |                              |      |
| AR        | 人脸图像数据集                                        |                              |      |
| SIFT Flow | 不同场景的图像数据集                                  |                              |      |
| ADE20K    | 不同场景的图像数据集                                  |                              |      |
| SegNet    | SegNet (3.5K dataset training - 140K)                 |                              |      |

------

算子开发 

Caffe ： 

   ./build/internal/bin/classification -f caffe -p /mnt/d/Project/Tengine-D/Operator_create/swish.prototxt 

  ./build/internal/bin/classification -f caffe -p /mnt/d/Project/Tengine-D/Operator_create/swish.prototxt

   ./build/tests/bin/test_caffe_swish /mnt/d/Project/Tengine-D/Operator_create/swish.prototxt /mnt/d/Project/Tengine-D/Operator_create/swish.txt 8 5 1

TensorFlow ：

./build/tests/bin/test_tf /mnt/d/Project/Tengine-D/Operator_create/swish_test.pb /mnt/d/Project/Tengine-D/Operator_create/swish.txt 8 5 1

./build/tests/bin/test_tf /mnt/d/Project/Tengine-D/Operator_create/elu_test.pb /mnt/d/Project/Tengine-D/Operator_create/elu.txt 8 5 1

./build/tests/bin/test_tf /mnt/d/Project/Tengine-D/Operator_create/relu_n1_to_1_test.pb /mnt/d/Project/Tengine-D/Operator_create/relu_n1_to_1.txt 5 6 4

gdb ./build/tests/bin/test_tf 

run /mnt/d/Project/Tengine-D/Operator_create/relu_n1_to_1_test.pb /mnt/d/Project/Tengine-D/Operator_create/relu_n1_to_1.txt 5 6 4

/mnt/d/Project/Tengine-D/Operator_create/elu_test.pb 

/mnt/d/Project/Tengine-D/Operator_create/relu_n1_to_1_test.pb

![img](https://app.yinxiang.com/FileSharing.action?hash=1/04261467714fefc36330938a83b1a151-8054)

时间：6月10号 - 23号 ，注意要保持对tensorFlow和caffe的兼容  

| 算子                        | 20190628                                    | 时间20190617进度                                             |      |
| --------------------------- | ------------------------------------------- | ------------------------------------------------------------ | ---- |
| RELU_N1_TO_1                | 完成，单个算子测试OK  带模型测试未测试      | 完成：实现完成，单个算子测试OK 待完成：融合到模型测试全面测试原平台（TensorFlow）的测试 |      |
| Elu                         | 完成，单个算子测试OK TensorFlow带模型测试OK | 完成：实现完成，单个算子测试OK 待完成：融合到模型测试全面测试原平台（TensorFlow）的测试 |      |
| SWISH                       | 完成，单个算子测试OK TensorFlow带模型测试OK | 完成：实现完成，单个算子测试OK 待完成：融合到模型测试全面测试原平台（TensorFlow）的测试 |      |
| **RESIZE_NEAREST_NEIGHBOR** | 算子实现中                                  |                                                              |      |
| EMBED                       |                                             | 未实现                                                       |      |
| **CRELU**                   |                                             | 未实现                                                       |      |

caffe测试单独的算子： https://www.cnblogs.com/jourluohua/p/10480195.html 

TensorFlow测试单独的算子： 

Tengine测试单独的算子： 

| TensorFlow的官网： https://tensorflow.google.cn/api_docs/python/tf?hl=en caffe官网： http://caffe.berkeleyvision.org/doxygen/index.htmlmxnet:  http://mxnet.incubator.apache.org/api/c++/index.html 算子定义：caffe的 TensorFlowmxnet的算子都在： |                                                              |                                                              |                                                              |                                                              |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| caffe                                                        | 算子主要在目录下：caffe\src\caffe\layers                     |                                                              |                                                              |                                                              |
| TensorFlow                                                   | 会在register.cc中进行注册                                    |                                                              |                                                              |                                                              |
| mxnet                                                        | mxnet\src\operator 目录下 在MXNet中一个算子是一个class，包括计算逻辑和优化辅助信息两个部分。 主要用MXNET_REGISTER_OP_PROPERTY注册到系统 所以搜索算子可如下操作：grep "MXNET_REGISTER_OP_PROPERTY" ./ -nr \| grep "算子名称" |                                                              |                                                              |                                                              |
|                                                              |                                                              |                                                              |                                                              |                                                              |
| 算子                                                         | TensorFlow                                                   | caffe                                                        | mxnet                                                        |                                                              |
| RELU_N1_TO_1                                                 | 源码register.cc: Register_RELU_N1_TO_1                       | 源码搜索没有                                                 | 源码搜索没有                                                 | ![img](https://app.yinxiang.com/FileSharing.action?hash=1/d53809e2583d65ccc1600d5bb883298c-2559) |
| **RESIZE_NEAREST_NEIGHBOR**                                  | 源码搜索：register.cc: Register_RESIZE_NEAREST_NEIGHBORAPI官网搜索：https://tensorflow.google.cn/versions/r1.9/api_docs/python/tf/image/resize_nearest_neighbor?hl=en | 源码搜索没有                                                 | 源码搜索没有                                                 | 将目标图像各点的像素值设为源图像中与其最近的点               |
| Elu                                                          | 源码register.cc: Register_ELU tensorflow.nn.elu              | 源码搜索没有                                                 | 源码有：class Elu(Base) 但是好像是在第三方onnx目录下的Python测试code | ![img](https://app.yinxiang.com/FileSharing.action?hash=1/8a759657c6d2b1711cb5dff321b053d5-1973) |
| **CRELU**                                                    | 源码搜索没有                                                 | 源码搜索没有                                                 | 源码搜索没有                                                 |                                                              |
| SWISH                                                        | 源码搜索没有                                                 | 源码：swish_layer.cpp                                        | 源码搜索没有                                                 | f(x) = x · sigmoid(x)                                        |
| EMBED                                                        | 源码搜索没有                                                 | 源码：embed_layer.cpp http://caffe.berkeleyvision.org/tutorial/layers/embed.html | 源码搜索没有                                                 | 矩阵间乘法                                                   |

RELU_N1_TO_1/RESIZE_NEAREST_NEIGHBOR 

Elu/CRELU/SWISH/EMBED 

RELU_N1_TO_1

activations_test.cc

TEST(FloatActivationsOpTest, Relu1) {

 FloatActivationsOpModel m(BuiltinOperator_RELU_N1_TO_1,

​              /*input=*/{TensorType_FLOAT32, {1, 2, 4, 1}});

 m.SetInput({

   0.0, -0.6, 0.2, -0.4, //

   0.3, -2.0, 1.1, -0.1, //

 });

 m.Invoke();

 EXPECT_THAT(m.GetOutput(), ElementsAreArray({

​                 0.0, -0.6, 0.2, -0.4, //

​                 0.3, -1.0, 1.0, -0.1, //

​               }));

}

\-------------------------------------

register.cc : Register_RELU_N1_TO_1（）

./tensorflow/lite/kernels/activations.cc:984:TfLiteRegistration* Register_RELU_N1_TO_1() 

TfLiteRegistration* Register_RELU_N1_TO_1() {

 static TfLiteRegistration r = {/*init=*/nullptr, /*free=*/nullptr,

​                 activations::GenericPrepare,

​                 activations::Relu1Eval};

 return &r;

}

\-------------------------------------------------------------------

activations.cc : 348 

TfLiteStatus Relu1Eval(TfLiteContext* context, TfLiteNode* node) {

 const TfLiteTensor* input = GetInput(context, node, 0);

 TfLiteTensor* output = GetOutput(context, node, 0);

 switch (input->type) {

  case kTfLiteFloat32: {

   optimized_ops::Relu1(GetTensorShape(input), GetTensorData<float>(input),

​              GetTensorShape(output),

​              GetTensorData<float>(output));

   return kTfLiteOk;

  } break;

  default:

   context->ReportError(context,

​              "Only float32 is supported currently, got %s.",

​              TfLiteTypeGetName(input->type));

   return kTfLiteError;

 }

}

\-----------------------------------

legacy_reference_ops.h :1146 

inline void Relu1(const float* input_data, const Dims<4>& input_dims,

​         float* output_data, const Dims<4>& output_dims) {

 Relu1(DimsToShape(input_dims), input_data, DimsToShape(output_dims),

​    output_data);

}

types.h:318 

// TODO(b/80418076): Move to legacy ops file, update invocations.

inline RuntimeShape DimsToShape(const tflite::Dims<4>& dims) {

 return RuntimeShape(

   {dims.sizes[3], dims.sizes[2], dims.sizes[1], dims.sizes[0]});

}

\-----------------------------------------------

reference_ops.h :265 

template <typename T>

inline void Relu1(const RuntimeShape& input_shape, const T* input_data,

​         const RuntimeShape& output_shape, T* output_data) {

 gemmlowp::ScopedProfilingLabel label("Relu1 (not fused)");

 const int flat_size = MatchingFlatSize(input_shape, output_shape);

 for (int i = 0; i < flat_size; ++i) {

  const T val = input_data[i];

  const T upper = 1;

  const T lower = -1;

  const T clamped = val > upper ? upper : val < lower ? lower : val;

  output_data[i] = clamped;

 }

}

对每一个输入，如果>1 ,则是1， 如果<-1 ,则是-1，其他的就是其本身 

![img](https://app.yinxiang.com/FileSharing.action?hash=1/d53809e2583d65ccc1600d5bb883298c-2559)

RESIZE_NEAREST_NEIGHBOR 

   https://tensorflow.google.cn/api_docs/python/tf/image/resize_nearest_neighbor 

https://tensorflow.google.cn/versions/r1.9/api_docs/python/tf/image/resize_nearest_neighbor?hl=en 

网上的说明：https://www.w3cschool.cn/tensorflow_python/tensorflow_python-u5bi2rgy.html

图片缩放的两种常见算法：

   1，最近邻域内插法(Nearest Neighbor interpolation)

   2，双向性内插法(bilinear interpolation)

最近邻域内插法： 顾名思义，就是将目标图像各点的像素值设为源图像中与其最近的点。算法优点简单、速度快。

![img](https://app.yinxiang.com/FileSharing.action?hash=1/7a5258dc3f7bf2e5ae25a0dd23d59895-241200)

tf.image.resize_nearest_neighbor(

  images,

  size,

  align_corners=False,

  name=None

)

\---------------------------------------------------------

resize_nearest_neighbor.cc 

TfLiteRegistration* Register_RESIZE_NEAREST_NEIGHBOR() {

\#ifdef USE_NEON

 return Register_RESIZE_NEAREST_NEIGHBOR_NEON_OPT();

\#else

 return Register_RESIZE_NEAREST_NEIGHBOR_GENERIC_OPT();

\#endif

}

TfLiteRegistration* Register_RESIZE_NEAREST_NEIGHBOR_GENERIC_OPT() {

 static TfLiteRegistration r = {

   nullptr, nullptr, resize_nearest_neighbor::Prepare,

   resize_nearest_neighbor::Eval<

​     resize_nearest_neighbor::kGenericOptimized>};

 return &r;

}

\-------------------------------------------------------------------------------

reference_ops.h 4310 

template <typename T>

inline void ResizeNearestNeighbor(

ElU

activations.cc ：970

TfLiteRegistration* Register_ELU() {

 static TfLiteRegistration r = {/*init=*/nullptr, /*free=*/nullptr,

​                 activations::GenericPrepare,

​                 activations::EluEval};

 return &r;

}

TfLiteStatus EluEval(TfLiteContext* context, TfLiteNode* node) {

 const TfLiteTensor* input = GetInput(context, node, 0);

 TfLiteTensor* output = GetOutput(context, node, 0);

 switch (input->type) {

  case kTfLiteFloat32: {

   optimized_ops::Elu(GetTensorShape(input), GetTensorData<float>(input),

​             GetTensorShape(output), GetTensorData<float>(output));

   return kTfLiteOk;

  } break;

  default:

   context->ReportError(context,

​              "Only float32 is supported currently, got %s.",

​              TfLiteTypeGetName(input->type));

   return kTfLiteError;

 }

}

\---------------------------------------------------------------------------

reference_ops.h 244

inline void Elu(const RuntimeShape& input_shape, const float* input_data,

​        const RuntimeShape& output_shape, float* output_data) {

 const int flat_size = MatchingFlatSize(input_shape, output_shape);

 for (int i = 0; i < flat_size; ++i) {

  const float val = input_data[i];

  output_data[i] = val < 0.0 ? std::exp(val) - 1 : val;

 }

}

![img](https://app.yinxiang.com/FileSharing.action?hash=1/8a759657c6d2b1711cb5dff321b053d5-1973)

CRELU 

  TensorFlow： https://tensorflow.google.cn/versions/r1.11/api_docs/python/tf/nn/crelu?hl=en 

tf.nn.crelu(

  features,

  name=None,

  axis=-1

)

SWISH 

激活函数： https://blog.csdn.net/uwr44uouqcnsuqb60zk2/article/details/78334072 

https://www.cnblogs.com/zhang-yd/p/7744016.html

Swish 激活函数：f(x) = x · sigmoid(x) 

![img](https://app.yinxiang.com/FileSharing.action?hash=1/b5da513e826c80f56fb305efd754b1f7-52472)

swish_layer.cpp 

template <typename Dtype>

void SwishLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,

  const vector<Blob<Dtype>*>& top) {

 const Dtype* bottom_data = bottom[0]->cpu_data();

 Dtype* sigmoid_input_data = sigmoid_input_->mutable_cpu_data();

 Dtype* top_data = top[0]->mutable_cpu_data();

 const int count = bottom[0]->count();

 Dtype beta = this->layer_param_.swish_param().beta();

 caffe_copy(count, bottom_data, sigmoid_input_data); // bottom_data -->sigmoid_input_data

 caffe_scal(count, beta, sigmoid_input_data); // sigmoid_input_data = sigmoid_input_data * beta 

   // bottom_data = bottom_data * beta

 sigmoid_layer_->Forward(sigmoid_bottom_vec_, sigmoid_top_vec_); 

  以上是对sigmoid的操作处理，综合起来处理就是对bottom_data做了sigmoid处理 ，然后下面再做一个乘法

 caffe_mul(count, bottom_data, sigmoid_output_->cpu_data(), top_data); //y[i] = a[i] * b[i]）

   //top_data = bottom_data * sigmoid_output_->cpu_data()

}

EMBED 

 官网参考： http://caffe.berkeleyvision.org/doxygen/classcaffe_1_1EmbedLayer.html 

[     http://caffe.berkeleyvision.org/tutorial/layers/embed.html](http://caffe.berkeleyvision.org/tutorial/layers/embed.html)

 

template <typename Dtype>

void EmbedLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,

  const vector<Blob<Dtype>*>& top) {

 const Dtype* bottom_data = bottom[0]->cpu_data();

 const Dtype* weight = this->blobs_[0]->cpu_data();

 Dtype* top_data = top[0]->mutable_cpu_data();

 int index;

 for (int n = 0; n < M_; ++n) {

  index = static_cast<int>(bottom_data[n]);

  DCHECK_GE(index, 0);

  DCHECK_LT(index, K_);

  DCHECK_EQ(static_cast<Dtype>(index), bottom_data[n]) << "non-integer input";

  caffe_copy(N_, weight + index * N_, top_data + n * N_);

 }

 if (bias_term_) {

  const Dtype* bias = this->blobs_[1]->cpu_data();

  caffe_cpu_gemm<Dtype>(CblasNoTrans, CblasNoTrans, M_, N_, 1, Dtype(1),

​    bias_multiplier_.cpu_data(), bias, Dtype(1), top_data); 

​     //  top_data = Dtype(1)*bias*bias_multiplier_.cpu_data() + top_data*Dtype(1)

 }

}

   caffe_cpu_gemm --》 cblas_sgemm  矩阵间乘法 参考： https://blog.csdn.net/qq_28660035/article/details/80520064

   cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K, alpha, A, A的列数, B, B的列数, beta, C, C的列数)  

void caffe_cpu_gemm<float>(const CBLAS_TRANSPOSE TransA,  const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,

  const float alpha, const float* A, const float* B, const float beta,  float* C) 

功能： C=alpha*A*B+beta*C

A,B,C 是输入矩阵（一维数组格式）

CblasRowMajor :数据是行主序的（二维数据也是用一维数组储存的）

TransA, TransB：是否要对A和B做转置操作（CblasTrans CblasNoTrans）

M： A、C 的行数

N： B、C 的列数

K： A 的列数， B 的行数

lda ： A的列数（不做转置）行数（做转置）

ldb： B的列数（不做转置）行数（做转置）



## GDB remote debug （gdb 远程调试）

### 0.保证在远程目标机上gdbserver 和本地机上使用的是同一版本gdb

***下载、编译arm-linux-gdb源码***

下载 http://ftp.gnu.org/gnu/gdb/

Linux系统本身已经自带gdb工具，但无法用在嵌入式调试中，需要单独编译arm-linux-gdb。
### 1 编译arm-linux-gdb　　
1).解压源码包

```bash
$ tar zxvf gdb-7.11.tar.gz 
$ cd gdb-7.11/
```
2) .生成Makefile

```bash
$ ./configure --target=arm-linux --prefix=$PWD/__install
```
–target：指定目标平台。–prefix：指定安装路径。
　
3).编译
```bash
$ make
```
4).安装
```bash
$ make install
$ sudo cp __install/bin/arm-linux-gdb /usr/bin/
```
执行此命令后会在当前目录下生成文件夹__install/里面包含可执行文件、头文件、动态库文件等，
将生成的arm-linux-gdb文件拷贝到系统/usr/bin/目录下，这样便可以在任何地方很方便的调用。
### 2. 编译gdbserver
2.1.生成Makefile

```bash
$ cd gdb/gdbserver/
$ ./configure --target=arm-linux --host=arm-linux-androideabi
```
–host：指定交叉工具链，arm-linux-gnueabi为我的目标系统的交叉工具链。

***注***：要想清理configure，使用make distclean　　
2.编译

```bash
$ make
```
编译gdbserver不需要执行make install命令，make之后在当前目录下会生成可执行程序gdbserver，将其拷贝到目标系统中。

原文链接：https://blog.csdn.net/zhaoxd200808501/article/details/77838933

### 3.调试过程

​	远程目标机ip是192.168.1.28，

​	本地宿主机ip是192.168.1.10.

#### 3.1 Target端建立远程调试服务

```bash
#start a session
openailab@DESKTOP: $ adb connect 192.168.1.28
openailab@DESKTOP: $ adb push instll /data/local/tmp
openailab@DESKTOP: $ adb shell cd /data/local/tmp
           rk3288: $ cd /data/local/tmp
```
在远程调试时，首先，需要在远程使用 gdbserver 打开要调试的程序，有两种方式：

1)直接用gdbserver打开：

```bash
           rk3288: $ gdbservr :1234 ./install/bin/tm_benchmark
```
2) attach 到已经在执行的进程上：

```bash
           rk3288: $ gdbserver --attach localhost:1234 16924
```
上面这句表示，用 attach 的方式打开 gdbserver，调试进程号(PID)为 16924 的程序。
> 其中IP地址为用来远程调试的上位机地址（现在直接被gdbserver忽略掉，所以可以不写），端口为target TCP 监听端口。目标程序不需要符号表，即可以是strip后的，这样可以节省存储空间，所有的符号信息处理都是在Host端的gdb处完成的。



#### 3.2 Host端GDB加载要调试的程序

这里要调试的程序得是交叉编译过的，并且加了-g参数。然后，在另外一边，使用相应版本的 gdb 来作为 client 与 gdbserver 连接即可

```bash
#start b session
openailab@DESKTOP: $ arm-linux-gdb ./../build-android32/install/bin/tm_benchmark
(gdb)  target remote 192.168.1.28:1234
(gdb)
```



至此，gdb 与 gdbserver 就勾搭上了，可以像在本地调试一样，对远程机器上的程序进行调试。

需要说明的是，在调试过程中，gdbserver 侧是不接受 ctrl-c 来终止程序的。

#### 3.4 Target端退出远程调试服务

退出 gdbserver 目前知道的就两种方法，在 gdb 侧执行 quit，或者在 remote 侧使用 killall。



