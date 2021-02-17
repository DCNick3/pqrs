# pqrs

A Portable QR Scanning library.
QR code detection is very much based on algorithms used in [BoofCV](https://boofcv.org).
Decoding is based on [zxing](https://github.com/zxing/zxing) 

Requires C++17 compiler. Seems to work with GCC and Clang

Compiles as native code and as webassembly code for browser usage using emscripten

### Why another QR scanner?

I needed a QR scanning library in web application. 
So, I went with [jsQR](https://github.com/cozmo/jsQR), which is one of [zxing](https://github.com/zxing/zxing)  ports to js.
What could go wrong? Apparently, a lot.

The scanner was used to scan an image projected to screen in intensively lit big auditorium.
Users were to scan it without leaving their places. And this actually posed a challenge...

I have distinguished two conditions that pose a challenge to zxing:

First of them is low projector image contrast, which is an inherent problem of all projectors.
They cannot make "black" any darker than ambient lighting, so the only way to increase contrast is increasing light output, which is limited.
   
This affects the stage of QR code detection. Most scanners begin with image binarization and then try to detect finder patterns.
Low contrast is quite a challenge for most binarization algorithms and, for no surprise, zxing struggled with it.
   
The second problem was due to the fact that auditorium was quite big and users were to scan QR without leaving their seats.
This required scanning from quite large angles: up to 70 degrees from normal to the screen. 
zxing does not handle huge perspective distortions caused by this quite well.

Then I surveyed already existing QR scanners to find those reasonably handling my problematic cases.

That's when I found [BoofCV](https://boofcv.org/), which boasted [surprising performance](https://boofcv.org/index.php?title=Performance:QrCode) in my cases.
And apparently, it was actually pretty good =). 
There was only one problem: it's written in Java, which is not straightforward to run in browser context.

So, I've decided to write a C++ port(-ish) of BoofCV QR scanner and compile it to webassembly using emscripten.

From this I got, on one hand, an ability to use it in much more languages than BoofCV (C++, arguably, has interoperability rate comparable to that of C) and performance (wasm is quite close to what you can get in native code).

It is not a direct port, some corners were cut (where it seemed like it would not affect performance or scanning quality), some features were added (like local thresholding when sampling QR code modules, which handled the uneven lighting).

