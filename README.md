# Computer Graphics Assignment 5 – Software Rasterizer

## Overview

이 과제의 목표는 **소프트웨어 렌더링 파이프라인**을 직접 구현하여, 여러 개의 삼각형으로 구성된 단일 구체(sphere)를 “Unshaded” 상태로 화면에 출력하는 것입니다.  
- **Input**: 수업 웹페이지에서 제공되는 `sphere_scene.cpp` (unit‐sphere 메시 생성)  
- **Pipeline**: 모델링 → 뷰 → 투영 → 뷰포트 → 래스터라이즈 → 깊이(depth) 버퍼 → BMP 출력  
- **Output**: `HW5_Result_Img.bmp` (512×512 해상도, 흰색 구체)

---

## Table of Contents

1. [Compilation Instructions](#compilation-instructions)  
2. [Run Instructions](#run-instructions)   
3. [Pipeline Summary](#pipeline-summary)  
4. [Components Details](#components-details)    

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
실행 후 작업 디렉터리에 HW5_Result_Img.bmp 파일이 생성됩니다.  
HW5_Result_Img.bmp 를 운영체제 기본 이미지 뷰어로 열어 Unshaded Sphere 결과를 확인하세요.

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
   - Back‐face culling  

5. **Rasterization**  
   - 삼각형별  
     1. Back-face culling  
     2. Bounding Box 계산  
     3. Barycentric 검사 & depth buffer  
     4. depth 테스트 후 픽셀 쓰기

6. **Output**  
   ```cpp
   saveBMP("HW5_Result_Img.bmp", 512, 512, frameBuffer);
   ```


## Component Details

### sphere_scene.cpp
- 전체 정점 수: `(height-2)*width + 2`  
- 전체 삼각형 수: `(height-2)*(width-1)*2`  
- 중간 위도 루프 → 단위 구체 정점 생성  
- 북·남극 정점 추가  
- 링·극점 연결용 인덱스 버퍼 채우기

### main.cpp
- **create_scene()** 호출 → 메시 데이터 준비  
- **버퍼 할당**: 색상 + 깊이  
- **행렬 계산**: `M`, `V`, `P`, `W`  
- **transform_vertex** 람다로 화면 좌표 변환  
- **래스터라이징**: 위 Pipeline Summary 순서 따름  
- **saveBMP**로 결과 이미지 출력

