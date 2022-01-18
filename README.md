# FBrowser
QtWebKit-based web browser on Linux framebuffer.

# Security Notes
This browser is not intended for visiting untrusted websites.

By default, OSBridge is enabled, which allows JavaScript to execute arbitrary code on the host system for accessing hardware resources and system services directly. This feature could be disabled in config.json.

Additionally, QtWebKit is based on an old WebKit revision with known unpatched security issues. In this case, it is strongly recommended to avoid using this browser with untrusted websites or transferring sensitive data.


# Features
1. A simple web browser that draws on framebuffer directly, does not require X server.
2. Low memory footprint, works well on devices with >= 128MB RAM.
3. Webkit-based, supports modern HTML5 features, 100/100 on ACID3 test.
4. No OpenGL implementation required, works on embedded processors that have no GPU or GPU driver is not available.
5. Packaged as a rootfs including all dependencies, could be used as a standalone OS or inside a chroot container on existing Linux distributions.
6. Built-in websocket server for remote management. 
7. Provides JSBridge/OSBridge API for controlling host system with JavaScript.
8. Compiled for armv6 and riscv64. Tested on Rpi Zero/Zero2 and Allwinner D1.


# License
This project is licensed under the LGPLv3 and commercial license.

Commercial license can be purchased by individual users or organizations.

Please note that you don't need to buy commercial license if your use of this project is compitable with LGPLv3 .
