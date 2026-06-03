Metal with C++ starter
======================

This repository gathers my learnings about Apple Metal C++ with a focus on using it for machine learning.
Personally, I would have preferred to get a nice step-by-step guide from someone else, but sadly I have not found a recent and detailed one.
Thus, I have now started to write my own.
In future, you can expect a blog post from me that will explain this a bit more.
To get started yourself, browse the code in the following order:

1. `src/list_devices.cpp`: fully self-contained example that lists your available Metal devices.

Development
-----------

You need to download [metal-cpp](https://developer.apple.com/metal/cpp/) to get the Metal C++ headers.

```
wget https://developer.apple.com/metal/cpp/files/metal-cpp_26.zip
unzip metal-cpp_26.zip
```

In addition, you should have the latest macOS SDK installed (via Xcode).

The remaining development setup is done using [pixi](https://prefix.dev/).
If you use `direnv`, everthing will be setup on activation.
Otherwise, you will need:

```
pixi install
pixi shell
pixi run meson-configure
pixi run meson-compile
```

Learnings
---------

1. Most Metal tutorials start with rendering / gaming setups. [Performing calculations on a GPU](https://developer.apple.com/documentation/metal/performing-calculations-on-a-gpu#Get-a-reference-to-the-Metal-function) was the simplest one I found (from Apple) that looks into computations.
2. Claude Opus 4.6 knows about Metal C++, but you really need to tell it to use `metal-cpp`. If you only ask it for using Metal in C++, it will just give you Objective-C.
3. Claude Opus 4.6 only knows about Metal 3, but at the time of writing, Metal 4 was already released for a while. You will need to that to take full advantage of an M5 GPU.
4. You can look up the feature of every Metal device at https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf

Disclaimer about sources
------------------------

The code here was written with Support of Claude Opus 4.6 as the documentation on `metal-cpp` is quite limited.
Given the information from Claude, we have used [mlx](https://github.com/ml-explore/mlx) as a way see acutal usage of the mentioned code snippets.

Glue infrastructure like the Meson files were built using DeepSeek-V4-Flash in [ds4](https://github.com/antirez/ds4) on an actual Metal device.
