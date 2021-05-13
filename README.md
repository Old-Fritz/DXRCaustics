This repo contains implementation of Caustic Visualization with the DirectX 12 

### DirectX 12 Ultimate sample

This sample demonstrates how DirectX Raytracing (DXR) brings a new level of graphics realism to video games, previously only achievable in the movie industry.

[MiniEngine Sample](src/ModelViewer/readme.md)
This sample demonstrates integration of the DirectX Raytracing in the MiniEngine's Model Viewer and several sample uses of raytracing.
![D3D12 Raytracing Mini Engine](Src/ModelViewer/Screenshot_small.png)

## MiniEngine: A DirectX 12 Engine Starter Kit

  Microsoft core library of helper classes and platform abstractions to be able to create a new app by writing just the Init(), Update(), and Render() functions and leveraging as much reusable code as possible.  Today this core library has been redesigned for DirectX 12 and aims to serve as an example of efficient API usage.  It is obviously not exhaustive of what a game engine needs, but it can serve as the cornerstone of something new.  You can also borrow whatever useful code you find.

### Some features of MiniEngine
* High-quality anti-aliased text rendering
* Real-time CPU and GPU profiling
* User-controlled variables
* Game controller, mouse, and keyboard input
* A friendly DirectXMath wrapper
* Perspective camera supporting traditional and reversed Z matrices
* Asynchronous DDS texture loading and ZLib decompression
* Large library of shaders
* Easy shader embedding via a compile-to-header system
* Easy render target, depth target, and unordered access view creation
* A thread-safe GPU command context system (WIP)
* Easy-to-use dynamic constant buffers and descriptor tables

## Requirements
* GPU and driver with support for [DirectX 12 Ultimate](http://aka.ms/DirectX12UltimateDev)

### Master branch
This branch is intended for the latest [released](https://docs.microsoft.com/en-us/windows/release-information/) Windows 10 version.
* Windows 10 version 2004 (no new features were added in version 20H2)
* [Visual Studio 2019](https://www.visualstudio.com/) with the [Windows 10 SDK version 2004(19041)](https://developer.microsoft.com/en-US/windows/downloads/windows-10-sdk)
