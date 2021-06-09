# README

对标 [rCore-Tutorial-v3](https://github.com/rcore-os/rCore-Tutorial-v3/) 的 C 版本代码。

主要参考 [xv6-riscv](https://github.com/mit-pdos/xv6-riscv)。

运行方式：

* 先checkout到分支(ch1-ch7)，切换到lab1-lab8的代码（ch7分支包含lab7和lab8）
* 于 user 目录 make, 得到用户文件(user/target/*.bin)
* 于 kernel 目录 make run，运行 os