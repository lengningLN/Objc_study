//
//  main.m
//  KCObjc
//
//  Created by Cooci on 2020/7/24.
//

#import <Foundation/Foundation.h>
#import "LGPerson.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        
        NSObject *objc1 = [[NSObject alloc] init];
        LGPerson *objc2 = [[LGPerson alloc] init];

        NSLog(@"Hello, World! %@ - %@",objc1,objc2);
    }
    return 0;
}



// iOS启动优化--探索load中方法替换迁移到initialize的可行性
//https://www.jianshu.com/p/62e6709ca171
