// main.cpp
#include <cstdint>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

// 1) sphere_scene.cpp 의 전역 선언 재사용
struct Vec3 { float x,y,z; };
extern int   gNumVertices;
extern int   gNumTriangles;
extern Vec3* gVertexArray;
extern int*  gIndexBuffer;
void create_scene();

// 2) 파이프라인용 수학 타입
struct Vec4 { float x,y,z,w; };
struct Mat4 { float m[4][4]; };

// 화면 버퍼 색 타입 (RGB24)
struct Color { uint8_t r,g,b; };

// 3) BMP 헤더 (packed)
#pragma region BMP

// 1 바이트 정렬로 구조체 크기·오프셋 고정
#pragma pack(push, 1)
struct BMPInfoHeader {
    uint32_t biSize       = sizeof(BMPInfoHeader);
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes     = 1;
    uint16_t biBitCount   = 24;
    uint32_t biCompression= 0;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter = 0;
    int32_t  biYPelsPerMeter = 0;
    uint32_t biClrUsed      = 0;
    uint32_t biClrImportant = 0;
};
struct BMPFileHeader {
    uint16_t bfType      = 0x4D42;
    uint32_t bfSize;
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits   = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
};
#pragma pack(pop)
#pragma endregion

// 4) BMP 저장 함수
void saveBMP(const char* fname, int NX, int NY, const std::vector<Color>& fb) {
    int rowBytes    = NX*3;
    int paddedBytes = (rowBytes + 3) & ~3;
    int dataSize    = paddedBytes * NY;

    BMPFileHeader fh;
    fh.bfSize = fh.bfOffBits + dataSize;
    BMPInfoHeader ih;
    ih.biWidth     = NX;
    ih.biHeight    = NY;
    ih.biSizeImage = dataSize;

    FILE* f = std::fopen(fname, "wb");
    std::fwrite(&fh,1,sizeof(fh),f);
    std::fwrite(&ih,1,sizeof(ih),f);

    std::vector<uint8_t> row(paddedBytes);
    for(int y=0; y<NY; ++y){
        int src = (NY-1-y)*NX;
        for(int x=0; x<NX; ++x){
            auto &c = fb[src + x];
            row[3*x+0] = c.b;
            row[3*x+1] = c.g;
            row[3*x+2] = c.r;
        }
        // 패딩
        for(int p=rowBytes; p<paddedBytes; ++p) row[p]=0;
        std::fwrite(row.data(),1,paddedBytes,f);
    }
    std::fclose(f);
}

// 5) 행렬·벡터 유틸
Mat4 mul(const Mat4&A,const Mat4&B){
    Mat4 R{};
    for(int i=0;i<4;++i) for(int j=0;j<4;++j){
        float s=0;
        for(int k=0;k<4;++k) s+=A.m[i][k]*B.m[k][j];
        R.m[i][j]=s;
    }
    return R;
}
Vec4 mul(const Mat4&A,const Vec4&v){
    return {
      A.m[0][0]*v.x + A.m[0][1]*v.y + A.m[0][2]*v.z + A.m[0][3]*v.w,
      A.m[1][0]*v.x + A.m[1][1]*v.y + A.m[1][2]*v.z + A.m[1][3]*v.w,
      A.m[2][0]*v.x + A.m[2][1]*v.y + A.m[2][2]*v.z + A.m[2][3]*v.w,
      A.m[3][0]*v.x + A.m[3][1]*v.y + A.m[3][2]*v.z + A.m[3][3]*v.w
    };
}
Mat4 Identity(){
    Mat4 I{};
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) I.m[i][j] = (i==j?1.0f:0.0f);
    return I;
}
Mat4 Translate(float tx,float ty,float tz){
    Mat4 M=Identity(); M.m[0][3]=tx; M.m[1][3]=ty; M.m[2][3]=tz; return M;
}
Mat4 Scale(float sx,float sy,float sz){
    Mat4 M=Identity(); M.m[0][0]=sx; M.m[1][1]=sy; M.m[2][2]=sz; return M;
}
Mat4 MakePerspective(float l,float r,float b,float t,float n,float f){
    Mat4 P{};
    P.m[0][0]=2*n/(r-l);   P.m[0][2]=(r+l)/(r-l);
    P.m[1][1]=2*n/(t-b);   P.m[1][2]=(t+b)/(t-b);
    P.m[2][2]=-(f+n)/(f-n); P.m[2][3]=-2*f*n/(f-n);
    P.m[3][2]=-1;
    return P;
}
Mat4 MakeFrustumFromZPlanes(float l,float r,float b,float t,float nz,float fz){
    // nz,fz 는 음의 z값 → 내부에서 양수 거리로 변환
    return MakePerspective(l,r,b,t,-nz,-fz);
}
Mat4 MakeViewport(int nx,int ny){
    Mat4 W=Identity();
    W.m[0][0]=nx/2.0f; W.m[0][3]=(nx-1)/2.0f;
    W.m[1][1]=ny/2.0f; W.m[1][3]=(ny-1)/2.0f;
    W.m[2][2]=0.5f;    W.m[2][3]=0.5f;
    return W;
}

int clampi(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }
float min3(float a,float b,float c){ return std::min(a,std::min(b,c)); }
float max3(float a,float b,float c){ return std::max(a,std::max(b,c)); }

void barycentric(const Vec4&v0,const Vec4&v1,const Vec4&v2,
                 float px,float py,
                 float&w0,float&w1,float&w2){
    float x0=v0.x,y0=v0.y, x1=v1.x,y1=v1.y, x2=v2.x,y2=v2.y;
    float d = (y1-y2)*(x0-x2)+(x2-x1)*(y0-y2);
    w0 = ((y1-y2)*(px-x2)+(x2-x1)*(py-y2)) / d;
    w1 = ((y2-y0)*(px-x2)+(x0-x2)*(py-y2)) / d;
    w2 = 1.0f-w0-w1;
}

// 6) main: 렌더링 + BMP 저장
int main(){
    // 메시 생성
    create_scene();
    printf("NumVertices=%d, NumTriangles=%d\n", gNumVertices, gNumTriangles);

    const int NX=512, NY=512;
    std::vector<Color>  frameBuffer(NX*NY,{0,0,0});
    std::vector<float>  depthBuffer(NX*NY,
                                    std::numeric_limits<float>::infinity());

    // 변환 행렬
    Mat4 M = mul( Translate(0,0,-7), Scale(2,2,2) );
    Mat4 V = Identity();
    Mat4 P = MakeFrustumFromZPlanes(
        -0.1f, +0.1f, -0.1f, +0.1f,
        -0.1f, -1000.0f
    );
    Mat4 W = MakeViewport(NX, NY);

    // M,V,P,W 를 캡처
    auto transform_vertex = [&](const Vec3& v)->Vec4{
        Vec4 v4{v.x,v.y,v.z,1.0f};
        Vec4 clip = mul(P, mul(V, mul(M, v4)));
        // 원근 나누기 → NDC
        Vec4 ndc;
        ndc.x = clip.x/clip.w;
        ndc.y = clip.y/clip.w;
        ndc.z = clip.z/clip.w;
        ndc.w = 1.0f;
        // 뷰포트 변환 → 화면 좌표
        return mul(W, ndc);
    };

    // 디버그: 첫 정점 스크린 좌표
    Vec4 t0 = transform_vertex(gVertexArray[0]);
    printf("Screen coord of v0: x=%.2f, y=%.2f, z=%.2f\n",
           t0.x,t0.y,t0.z);

    int drawn = 0;
    // 래스터화
    for(int tri=0; tri<gNumTriangles; ++tri){
        int i0=gIndexBuffer[3*tri+0],
            i1=gIndexBuffer[3*tri+1],
            i2=gIndexBuffer[3*tri+2];
        Vec4 v0 = transform_vertex(gVertexArray[i0]);
        Vec4 v1 = transform_vertex(gVertexArray[i1]);
        Vec4 v2 = transform_vertex(gVertexArray[i2]);

        //  Back‐face Culling (screen‐space)
        // v0→v1, v0→v2 두 벡터의 외적 Z값(=signed area) 계산
        float ex1 = v1.x - v0.x;
        float ey1 = v1.y - v0.y;
        float ex2 = v2.x - v0.x;
        float ey2 = v2.y - v0.y;
        float crossZ = ex1 * ey2 - ey1 * ex2;
        // 모델이 CW 순서라면, crossZ >= 0 은 뒤집힌 면
        if (crossZ >= 0.0f) {
            continue;
        }

        // Bounding Box 계산
        int xmin=clampi(int(floor(min3(v0.x,v1.x,v2.x))),0,NX-1);
        int xmax=clampi(int(ceil (max3(v0.x,v1.x,v2.x))),0,NX-1);
        int ymin=clampi(int(floor(min3(v0.y,v1.y,v2.y))),0,NY-1);
        int ymax=clampi(int(ceil (max3(v0.y,v1.y,v2.y))),0,NY-1);

        for(int y=ymin; y<=ymax; ++y){
            for(int x=xmin; x<=xmax; ++x){
                float w0,w1,w2;
                barycentric(v0,v1,v2, x+0.5f,y+0.5f, w0,w1,w2);
                if(w0<0||w1<0||w2<0) continue;
                float z = w0*v0.z + w1*v1.z + w2*v2.z;
                int idx = y*NX + x;
                if(z < depthBuffer[idx]){
                    depthBuffer[idx]=z;
                    frameBuffer[idx]={255,255,255};
                    ++drawn;
                }
            }
        }
    }
    printf("Total pixels drawn: %d\n", drawn);

    // BMP 저장
    saveBMP("HW5_Result_Img.bmp", NX, NY, frameBuffer);

    return 0;
}
