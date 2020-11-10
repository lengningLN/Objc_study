一、App的启动过程（https://blog.csdn.net/tugele/article/details/81609604）

    1. app启动过程中，首先是操作系统内核（XNU）进行一些处理，比如新建进程、分配内存等。在XUN完成相关工作后，
        会将控制权交给动态链接器（dyld）用于加载动态库。从XNU到dyld，完成了一次从内核态到用户态的切换。
    2. 动态链接器（dyld）做了哪些事情呢？
        2.1 dyld的入口函数是_dyld_start，内部调用dyldbootstrap::start()函数，此函数会完成动态库加载过程，
        并返回主程序main函数入口。

        2.2 在dyldbootstrap::start()内部调用了调用dyld中的_main()函数，_main()函数返回主程序的
        main函数入口，也就是我们App的main函数地址

        2.3 _main()做了哪些事？
        主要完成上下文的建立。主程序初始化成imageLoader对象，加载共享的系统动态库，加载依赖的动态库，链接动态库
        初始化主程序，返回程序main()函数的地址。
    
            2.3.1 instantiateFromLoadedImage()
            此函数主要是将主程序Mach-O文件转变成了一个ImageLoader对象，用于后续的链接过程。ImageLoader
            是一个抽象类，和其相关的类有ImageLoaderMachO，ImageLoaderMachO是ImageLoader的子类，
            ImageLoaderMachO又有两个子类，分别是ImageLoaderMachOCompressed和ImageLoaderMachOClassic

            app启动过程中，主程序和其相关的动态库都会被转化成一个imageLoader对象。

            2.3.2 instantiateFromLoadedImage()函数中会检测mach-o文件的cputype和cpusubtype是否与当前系统兼容，
            之后调用instantiateMainExecutable()函数，根据Mach-O文件是否被压缩过，分别调用了
            ImageLoaderMachOCompressed::instantiateMainExecutable()和
            ImageLoaderMachOClassic::instantiateMainExecutable()。
            现在的Mach-O文件都是被压缩过的，因此我们只看一下ImageLoaderMachOCompressed::instantiateMainExecutable的实现。

            2.3.3 ImageLoaderMachOCompressed::instantiateMainExecutable（） 根据macho_header，返回一个
            ImageLoaderMachOCompressed对象。

            2.4.4 mapSharedCache()函数
            此负责将系统中的共享动态库加载进内存空间，比如UIKit就是动态共享库，这也是不同的App之间能够实现动态库共享的机制。
            不同App间访问的共享库最终都映射到了同一块物理内存，从而实现了共享动态库。
            mapSharedCache()中调用了内核中的一些方法，最终实际上是做了系统调用。mapSharedCache()
            的主要逻辑就是：先判断共享动态库是否已经映射到内存中了，如果已经存在，则直接返回；否则打开缓存文件，
            并将共享动态库映射到内存中。


            2.4.5 loadInsertedDylib(const char* path)函数
            共享动态库映射到内存后，dyld会把app 环境变量DYLD_INSERT_LIBRARIES中的动态库调用loadInsertedDylib()
            函数进行加载。可以在xcode中设置环境变量，打印出app启动过程中的DYLD_INSERT_LIBRARIES环境变量，
            这里看一下我们开发的app的DYLD_INSERT_LIBRARIES环境变量：

            DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/Library/CoreSimulator/Profiles/Runtimes/iOS.simruntime/Contents/Resources/RuntimeRoot/usr/lib/libBacktraceRecording.dylib:/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/Library/CoreSimulator/Profiles/Runtimes/iOS.simruntime/Contents/Resources/RuntimeRoot/usr/lib/libMainThreadChecker.dylib:/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/Library/CoreSimulator/Profiles/Runtimes/iOS.simruntime/Contents/Resources/RuntimeRoot/Developer/Library/PrivateFrameworks/DTDDISupport.framework/libViewDebuggerSupport.dylib

            loadInsertedDylib()函数中主要是调用了load()函数，看一下load()函数的实现：

            2.4.6 load()函数是查找动态库的入口，在load()函数中，会调用loadPhase0,loadPhase1,
            loadPhase2,loadPhase3,loadPhase4,loadPhase5,loadPhase6,对动态库进行查找。
            最终在loadPhase6中，对mach-o文件进行解析，并最终转成一个ImageLoader对象。
            看一下loadPhase6中的实现逻辑：

            2.4.7 loadPhase6中使用了ImageLoaderMachO::instantiateFromFile()
            函数来生成ImageLoader对象，ImageLoaderMachO::instantiateFromFile()
            的实现和上面提到的instantiateMainExecutable实现逻辑类似，也是先判断mach-o
            文件是否被压缩过，然后根据是否被压缩，生成不同的ImageLoader对象，这里不做过多的介绍。

            link
            在将主程序以及其环境变量中的相关动态库都转成ImageLoader对象之后，dyld会将这些ImageLoader
            链接起来，链接使用的是ImageLoader自身的link()函数。看一下具体的代码实现：
            
            link()函数中主要做了以下的工作：
            1. recursiveLoadLibraries递归加载所有的依赖库
            2. recursiveRebase递归修正自己和依赖库的基址
            3. recursiveBind递归进行符号绑定

            在递归加载所有的依赖库的过程中，加载的方法是调用loadLibrary()函数，实际最终调用的还是load()方法。
            经过link()之后，主程序以及相关依赖库的地址得到了修正，达到了进程可用的目的。

            2.4.8 initializeMainExecutable（）
            link()函数执行完毕后，会调用initializeMainExecutable()函数，可以将该函数理解为一个初始化函数。
            实际上，一个app启动的过程中，除了dyld做一些工作外，还有一个重要的角色，就是runtime，
            而且runtime和dyld是紧密联系的。runtime里面注册了一些dyld的回调通知，这些通知是在runtime
            初始化的时候注册的。其中有一个通知是，当有新的镜像加载时，会执行runtime中的load-images()
            函数。接下来看一些runtime中的源码，分析一下load-images()函数做了哪些操作。

            load_images()中首先调用了prapare_load_methods()函数，接着调用了call_load_methods()函数。看一下parpare_load_methods()的实现：
            _getObjc2NonlazyClassList获取到了所有类的列表，而remapClass是取得该类对应的指针，
            然后调用了schedule_class_load()函数，看一下schedule_class_load的实现：
            
            分析这段代码，可以知道，在将子类添加到加载列表之前，其父类一定会优先加载到列表中。
            这也是为何父类的+load方法在子类的+load方法之前调用的根本原因
        
            然后我们在看一下call_load_methods()函数的实现：
            call_load_methods中主要调用了call_class_loads()函数，看一下call_class_loads的实现：
            其主要逻辑就是从待加载的类列表loadable_classes中寻找对应的类，
            然后找到@selector(load)的实现并执行。

            getThreadPC
            getThreadPC是ImageLoaderMachO中的方法，主要功能是获取app main函数的地址，看一下其实现逻辑：
            该函数的主要逻辑就是遍历loadCommand，找到’LC_MAIN’指令，得到该指令所指向的便宜地址，
            经过处理后，就得到了main函数的地址，将此地址返回给__dyld_start。__dyld_start中将main函数
            地址保存在寄存器后，跳转到对应的地址，开始执行main函数，至此，一个app的启动流程正式完成。

总结：
在上面，已经将_main函数中的每个流程中的关键函数都介绍完了，最后，我们看一下_main函数的实现：
uintptr_t
_main(const macho_header* mainExecutableMH, uintptr_t mainExecutableSlide,
        int argc, const char* argv[], const char* envp[], const char* apple[],
        uintptr_t* startGlue)
{
    uintptr_t result = 0;
    sMainExecutableMachHeader = mainExecutableMH;
    // 处理环境变量，用于打印
    if ( sEnv.DYLD_PRINT_OPTS )
        printOptions(argv);
    if ( sEnv.DYLD_PRINT_ENV )
        printEnvironmentVariables(envp);
    try {
        // 将主程序转变为一个ImageLoader对象
        sMainExecutable = instantiateFromLoadedImage(mainExecutableMH, mainExecutableSlide, sExecPath);
        if ( gLinkContext.sharedRegionMode != ImageLoader::kDontUseSharedRegion ) {
            // 将共享库加载到内存中
            mapSharedCache();
        }
        // 加载环境变量DYLD_INSERT_LIBRARIES中的动态库，使用loadInsertedDylib进行加载
        if  ( sEnv.DYLD_INSERT_LIBRARIES != NULL ) {
            for (const char* const* lib = sEnv.DYLD_INSERT_LIBRARIES; *lib != NULL; ++lib)
                loadInsertedDylib(*lib);
        }
        // 链接
        link(sMainExecutable, sEnv.DYLD_BIND_AT_LAUNCH, true, ImageLoader::RPathChain(NULL, NULL), -1);
        // 初始化
        initializeMainExecutable();
        // 寻找main函数入口
        result = (uintptr_t)sMainExecutable->getThreadPC();
    }
    return result;
}


本篇文章介绍了从dyld处理主程序Mach-O开始，一直到寻找到主程序Mach-O
main函数地址的整个流程。需要注意的是，这也仅仅是一个大概流程的介绍，实际上，除了文章中所写的这些，源码中还有非常多的
细节处理，以及一些没有介绍到的知识点。无论是XNU，还是dyld，阅读其源码都是一个巨大的工程，需要在日后不断的学习、回顾。




