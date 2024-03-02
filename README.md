# Loft

Bachelor's project.

![banner](banner.png)

## Motivation

Toy renderer using Vulkan and it's full capabilities.

## Modules

### Base

Mostly some common classes across modules. You won't go far without this one.

### FrameGraph

Frame graph implementation inspired by a nice talk by EA, about their implementation of FrameGraph in FrostBite.

### Window

Very basic implementation of windowing system. Will be improved in the future.

## Building

To initialize and build everything into `./build/` directory, use 

```bash
$ cmake -S . -B build
$ cmake --build build 
```


There is one example prepared, the viewer. It takes the path to some glTF file as it's first argument.

If you have built everything into `./build/` directory, use

```bash
$ ./build/examples/viewer/loft_viewer ~/path/to/some/file.gltf
```

I recommend this public repository to try out some glTF models:
https://github.com/KhronosGroup/glTF-Sample-Assets

Tested assets are:
- Sponza
- SciFi Helmet
