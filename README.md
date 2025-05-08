# Computer Graphics Assignment 5 – Software Rasterizer

## Overview

이 과제의 목표는 **소프트웨어 렌더링 파이프라인**을 직접 구현하여, 여러 개의 삼각형으로 구성된 단일 구체(sphere)를 “Unshaded” 상태로 화면에 출력하는 것입니다.  
- **Input**: 수업 웹페이지에서 제공되는 `sphere_scene.cpp` (unit‐sphere 메시 생성)  
- **Pipeline**: 모델링 → 뷰 → 투영 → 뷰포트 → 래스터라이즈 → 깊이(depth) 버퍼 → BMP 출력  
- **Output**: `out.bmp` (512×512 해상도, 흰색 구체)

---

## Table of Contents

1. [Compilation Instructions](#compilation-instructions)  
2. [Run Instructions](#run-instructions)  
3. [File Structure](#file-structure)  
4. [Pipeline Summary](#pipeline-summary)  
5. [Components Description](#components-description)  
6. [Notes](#notes)  

---

## Compilation Instructions

터미널(또는 PowerShell)에서 아래 명령어로 컴파일하세요:

```bash
g++ -std=c++17 main.cpp sphere_scene.cpp -o rasterizer -O2
```

## Run Instructions

컴파일 후 생성된 실행 파일 rasterizer 를 실행합니다:
```bash
./rasterizer
```
실행 후 작업 디렉터리에 out.bmp 파일이 생성됩니다.
out.bmp 를 운영체제 기본 이미지 뷰어(Windows 사진 뷰어, macOS 미리보기 등)로 열어 Unshaded Sphere 결과를 확인하세요.

## File Structure

project-root/
├─ sphere_scene.cpp   # unit‐sphere 메시 생성(create_scene)
├─ main.cpp           # 변환 파이프라인 · 래스터라이저 · BMP 저장
└─ out.bmp            # 실행 결과 이미지 (자동 생성)


## Pipeline Summary

1. **Scene Generation**  
   - `create_scene()` 호출 →  
     - `gVertexArray` (정점 배열)  
     - `gIndexBuffer` (삼각형 인덱스)  
     - `gNumVertices`, `gNumTriangles` 초기화  

2. **Buffer Initialization**  
   - `frameBuffer`: RGB24 픽셀 버퍼 (512×512), 초기값 `{0,0,0}`  
   - `depthBuffer`: float 깊이 버퍼 (512×512), 초기값 `+∞`  

3. **Transformation Matrices**  
   - Modeling: `Scale(2,2,2)` → `Translate(0,0,-7)`  
   - View: `Identity()`  
   - Projection: `MakeFrustumFromZPlanes(-0.1, +0.1, -0.1, +0.1, -0.1, -1000.0)`  
   - Viewport: `MakeViewport(512, 512)`  

4. **Vertex Processing**  
   1. 모델 → 뷰 → 투영 변환  
   2. 원근 나누기 (perspective divide) → NDC  
   3. 뷰포트 변환 → 화면 좌표  

5. **Rasterization**  
   - 각 삼각형 순회  
   - 화면 공간에서 바운딩 박스 계산  
   - 바리센트릭 테스트 → 깊이 보간 → depthBuffer 비교 후 frameBuffer 갱신  
   - (선택) Back‐face culling  

6. **Output**  
   ```cpp
   saveBMP("out.bmp", 512, 512, frameBuffer);


## Components Description

### sphere_scene.cpp

- `create_scene()`
- `(height-2)*width + 2` 개 정점 수 계산
- `(height-2)*(width-1)*2` 개 삼각형 수 계산
- `new Vec3[...]`, `new int[...]` 로 정점·인덱스 버퍼 할당
- 위도/경도 루프를 통해 단위 구체 정점 좌표 생성
- 북극·남극 정점 추가
- 링과 극점을 연결하는 삼각형 인덱스 채우기

### main.cpp

1. **Scene Generation**  
   ```cpp
   create_scene();
   ```


2. **Buffers**
  ```cpp
  frameBuffer.assign(NX*NY, Color{0,0,0});
  depthBuffer.assign(NX*NY, std::numeric_limits<float>::infinity());
  ```
3. **Matrices**

Mat4 M = Translate(0,0,-7) * Scale(2,2,2);
Mat4 V = Identity();
Mat4 P = MakeFrustumFromZPlanes(-0.1f,+0.1f,-0.1f,+0.1f,-0.1f,-1000.0f);
Mat4 W = MakeViewport(512, 512);

4. **Vertex Shader (Lambda)**

auto transform_vertex = [&](const Vec3& v){
  Vec4 clip = P * V * M * Vec4{v.x,v.y,v.z,1};
  Vec4 ndc  = { clip.x/clip.w, clip.y/clip.w, clip.z/clip.w, 1 };
  return W * ndc;
};


5. **Rasterizer**

  1. 삼각형별 바운딩 박스 계산  
  2. 바리센트릭 검사 → 깊이 보간  
  3. depthBuffer 비교 후 frameBuffer 갱신  
  4. (선택) Back‐face culling  


6. **BMP Saving**

saveBMP("out.bmp", 512, 512, frameBuffer);
