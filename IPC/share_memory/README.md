# 15-31
## compile:
```
gcc 15-31.c -g -o 15-31
```

## debug:
```
gdb 15-31.c
b line_num
r
```
执行结果：
```
Array from 0x6010a0 to 0x60ace0
stack around 0x7ffdd23a2bac
malloced from 0x6b1010 to 0x6c96b0
share memory from 0x7f7c10049000 to 0x7f7c100616a0
```
可以发现共享内存的地址是和段紧挨着的

# 15-33
## compile
```
gcc 15-33.c -g -o 15-33
```