
## 利用信号量机制实现自旋锁

编译时加`pthread`选项
```
gcc -pthread xxx.c -g -o xxx
```
## gdb调试多进程命令
`set detach-on-fork off`: 其他进程挂起

`info inferiors`: 查看当前所有进程

`inferior process_num`: 切换进程
