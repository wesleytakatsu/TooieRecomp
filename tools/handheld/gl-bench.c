// Cost of each kind of state change per draw on the device's GL driver. Time is on the calling thread, flush excluded and shown apart.
//   gl-bench [driver library, default libmali.so.1]
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef void *P;
#define L(ret, name, ...) typedef ret (*name##_t)(__VA_ARGS__); static name##_t name;
L(P, eglGetDisplay, P)
L(int, eglInitialize, P, int *, int *)
L(int, eglBindAPI, unsigned)
L(int, eglChooseConfig, P, const int *, P *, int, int *)
L(P, eglCreateContext, P, P, P, const int *)
L(P, eglCreatePbufferSurface, P, P, const int *)
L(int, eglMakeCurrent, P, P, P, P)
L(int, eglGetError, void)
L(P, eglGetProcAddress, const char *)
L(unsigned, glCreateShader, unsigned)
L(void, glShaderSource, unsigned, int, const char *const *, const int *)
L(void, glCompileShader, unsigned)
L(void, glGetShaderiv, unsigned, unsigned, int *)
L(void, glGetShaderInfoLog, unsigned, int, int *, char *)
L(unsigned, glCreateProgram, void)
L(void, glAttachShader, unsigned, unsigned)
L(void, glLinkProgram, unsigned)
L(void, glGetProgramiv, unsigned, unsigned, int *)
L(void, glGetProgramInfoLog, unsigned, int, int *, char *)
L(void, glUseProgram, unsigned)
L(int, glGetUniformLocation, unsigned, const char *)
L(void, glUniform1ui, int, unsigned)
L(void, glUniform1i, int, int)
L(void, glUniform2f, int, float, float)
L(void, glGenTextures, int, unsigned *)
L(void, glBindTexture, unsigned, unsigned)
L(void, glActiveTexture, unsigned)
L(void, glTexStorage2D, unsigned, int, unsigned, int, int)
L(void, glTexSubImage2D, unsigned, int, int, int, int, int, unsigned, unsigned, const void *)
L(void, glTexParameteri, unsigned, unsigned, int)
L(void, glGenFramebuffers, int, unsigned *)
L(void, glBindFramebuffer, unsigned, unsigned)
L(void, glFramebufferTexture2D, unsigned, unsigned, unsigned, unsigned, int)
L(void, glGenBuffers, int, unsigned *)
L(void, glBindBuffer, unsigned, unsigned)
L(void, glBufferData, unsigned, intptr_t, const void *, unsigned)
L(void, glBindBufferBase, unsigned, unsigned, unsigned)
L(void, glBindBufferRange, unsigned, unsigned, unsigned, intptr_t, intptr_t)
L(void, glGenVertexArrays, int, unsigned *)
L(void, glBindVertexArray, unsigned)
L(void, glEnableVertexAttribArray, unsigned)
L(void, glVertexAttribPointer, unsigned, int, unsigned, unsigned char, int, const void *)
L(void, glDrawElements, unsigned, int, unsigned, const void *)
L(void, glDrawElementsInstancedBaseVertex, unsigned, int, unsigned, const void *, int, int)
L(void, glDrawRangeElements, unsigned, unsigned, unsigned, int, unsigned, const void *)
L(void, glBufferSubData, unsigned, intptr_t, intptr_t, const void *)
L(void, glViewport, int, int, int, int)
L(void, glScissor, int, int, int, int)
L(void, glEnable, unsigned)
L(void, glDisable, unsigned)
L(void, glBlendFuncSeparate, unsigned, unsigned, unsigned, unsigned)
L(void, glDepthFunc, unsigned)
L(void, glDepthMask, unsigned char)
L(void, glClear, unsigned)
L(void, glFlush, void)
L(void, glFinish, void)
L(unsigned, glGetError, void)
L(const unsigned char *, glGetString, unsigned)

static const char *vertexSource =
    "#version 310 es\n"
    "layout(location = 0) in highp vec2 pos;\n"
    "uniform highp vec2 offset;\n"
    "void main() { gl_Position = vec4(pos + offset, 0.5, 1.0); }\n";

static char pixelSource[8192];

static void makePixelSource(int samplers) {
    char *p = pixelSource;
    p += sprintf(p, "#version 310 es\nprecision highp float;\nprecision highp int;\nlayout(location = 0) out vec4 color;\nuniform uint renderIndex;\n");
    p += sprintf(p, "layout(std430, binding = 0) readonly buffer A { vec4 a[]; };\nlayout(std430, binding = 1) readonly buffer B { vec4 b[]; };\nlayout(std430, binding = 2) readonly buffer C { vec4 c[]; };\nlayout(std430, binding = 3) readonly buffer D { vec4 d[]; };\nlayout(std140, binding = 0) uniform E { vec4 e; };\n");
    p += sprintf(p, "layout(binding = 0) uniform sampler2D t[%d];\n", samplers);
    p += sprintf(p, "void main() { uint r = renderIndex & 7u; vec4 v = a[r] + b[r] + c[r] + d[r] + e;\n");
    for (int i = 0; i < samplers; i++) {
        p += sprintf(p, "  if (r == %du) v += texelFetch(t[%d], ivec2(gl_FragCoord.xy) & ivec2(7), 0);\n", i & 7, i);
    }
    p += sprintf(p, "  color = v * 0.1; }\n");
}

static const char *vertexSource3 =
    "#version 310 es\n"
    "layout(location = 0) in highp vec2 pos;\n"
    "layout(location = 1) in highp vec2 tc;\n"
    "layout(location = 2) in highp vec4 col;\n"
    "uniform highp vec2 offset;\n"
    "void main() { gl_Position = vec4(pos + offset + tc * 0.0001, 0.5, 1.0 + col.x * 0.0001); }\n";

static unsigned compile(unsigned type, const char *text) {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &text, NULL);
    glCompileShader(shader);
    int status = 0;
    glGetShaderiv(shader, 0x8B81, &status);
    if (!status) { static char log[4096]; glGetShaderInfoLog(shader, sizeof(log), NULL, log); printf("compile failed: %s\n", log); exit(2); }
    return shader;
}

static const char *currentVertexSource = NULL;

static unsigned makeProgram(int samplers) {
    makePixelSource(samplers);
    unsigned program = glCreateProgram();
    glAttachShader(program, compile(0x8B31, (currentVertexSource != NULL) ? currentVertexSource : vertexSource));
    glAttachShader(program, compile(0x8B30, pixelSource));
    glLinkProgram(program);
    int status = 0;
    glGetProgramiv(program, 0x8B82, &status);
    if (!status) { static char log[4096]; glGetProgramInfoLog(program, sizeof(log), NULL, log); printf("link failed: %s\n", log); exit(2); }
    glUseProgram(program);
    char name[16];
    for (int i = 0; i < samplers; i++) {
        snprintf(name, sizeof(name), "t[%d]", i);
        glUniform1i(glGetUniformLocation(program, name), i);
    }
    return program;
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

enum { DRAWS = 400, FRAMES = 25 };
static unsigned programs[3], textures[64], framebuffers[2], ssbo[2], ubo[2];
static int locIndex[3], locOffset[3];

enum Change {
    None, Uniform, Uniform3, OneTexture, FourTextures, AllTextures, TwoPrograms, Scissor, BlendToggle, DepthToggle, FramebufferSwap, SsboRange, UboRange,
    UniformTextureEveryThird, RealMix, Count
};
static const char *names[] = {
    "draw only", "1 uniform", "3 uniforms", "1 texture", "4 textures", "all textures", "program switch", "scissor", "blend on/off", "depth mask on/off",
    "same-attachment FBO swap", "SSBO range", "UBO range", "uniform, textures every 3rd", "uniform, pipeline+textures every 3rd"
};

static void bindAll(int samplers, int base) {
    for (int i = 0; i < samplers; i++) {
        glActiveTexture(0x84C0 + i);
        glBindTexture(0x0DE1, textures[(base + i) % 64]);
    }
}

static void run(enum Change change, int program, int samplers, const char *label) {
    double spent = 0, flushed = 0;
    glUseProgram(programs[program]);
    bindAll(samplers, 0);
    glUniform1ui(locIndex[program], 0);
    for (int frame = 0; frame < FRAMES; frame++) {
        glBindFramebuffer(0x8D40, framebuffers[0]);
        glDisable(0x0C11);
        glClear(0x4100);
        glEnable(0x0C11);
        glScissor(0, 0, 304, 228);
        double start = now();
        for (int d = 0; d < DRAWS; d++) {
            switch (change) {
            case None: break;
            case Uniform: glUniform1ui(locIndex[program], d); break;
            case Uniform3: glUniform1ui(locIndex[program], d); glUniform2f(locOffset[program], (d & 15) * 0.01f, (d & 7) * 0.01f); break;
            case OneTexture: glActiveTexture(0x84C0); glBindTexture(0x0DE1, textures[d % 64]); break;
            case FourTextures: for (int i = 0; i < 4; i++) { glActiveTexture(0x84C0 + i); glBindTexture(0x0DE1, textures[(d + i) % 64]); } break;
            case AllTextures: bindAll(samplers, d); break;
            case TwoPrograms: glUseProgram(programs[d & 1]); break;
            case Scissor: glScissor(0, 0, 304 - (d & 1), 228); break;
            case BlendToggle: if (d & 1) { glEnable(0x0BE2); } else { glDisable(0x0BE2); } break;
            case DepthToggle: glDepthMask((d & 1) ? 1 : 0); break;
            case FramebufferSwap: glBindFramebuffer(0x8D40, framebuffers[d & 1]); break;
            case SsboRange: glBindBufferRange(0x90D2, 0, ssbo[d & 1], 0, 1024); break;
            case UboRange: glBindBufferRange(0x8A11, 0, ubo[d & 1], 0, 256); break;
            case UniformTextureEveryThird:
                glUniform1ui(locIndex[program], d);
                if ((d % 3) == 0) { bindAll(samplers, d); }
                break;
            case RealMix:
                if ((d % 3) == 0) { glUseProgram(programs[(d / 3) & 1]); bindAll(samplers, d); }
                glUniform1ui(locIndex[(d / 3) & 1], d);
                break;
            default: break;
            }
            glDrawElements(0x0004, 6, 0x1403, 0);
        }
        spent += now() - start;
        double flushStart = now();
        glFlush();
        flushed += now() - flushStart;
        glFinish();
    }
    printf("%-38s %-12s %7.1f us per draw, flush %6.0f us per frame, error 0x%x\n", names[change], label, spent / (DRAWS * FRAMES), flushed / FRAMES, glGetError());
    fflush(stdout);
}

// What a real draw has that the loop above doesn't: another draw call, its own indices in a large buffer written each frame, three attribute streams.
enum DrawCall { Plain, InstancedBase, Ranged };
static unsigned vaoOne, vaoThree, program3, bigVertex[3], bigIndex16, bigIndex32;
static uint32_t bigIndices32[DRAWS * 24];
static uint16_t bigIndices16[DRAWS * 24];
static float bigStreams[3][DRAWS * 24 * 4];

static void runDrawKind(const char *label, enum DrawCall call, int three, int wide, int big, int upload, int vertexUpload) {
    if ((call == InstancedBase) && (glDrawElementsInstancedBaseVertex == NULL)) { printf("%-46s no function\n", label); return; }
    const unsigned program = three ? program3 : programs[0];
    const int loc = glGetUniformLocation(program, "renderIndex");
    glUseProgram(program);
    bindAll(16, 0);
    glBindVertexArray(three ? vaoThree : vaoOne);
    if (big) { glBindBuffer(0x8893, wide ? bigIndex32 : bigIndex16); }
    double spent = 0, uploaded = 0;
    for (int frame = 0; frame < FRAMES; frame++) {
        glBindFramebuffer(0x8D40, framebuffers[0]);
        glDisable(0x0C11);
        glClear(0x4100);
        glEnable(0x0C11);
        glScissor(0, 0, 304, 228);
        double uploadStart = now();
        if (upload) { glBufferSubData(0x8893, 0, wide ? sizeof(bigIndices32) : sizeof(bigIndices16), wide ? (const void *)bigIndices32 : (const void *)bigIndices16); }
        if (vertexUpload) {
            for (int i = 0; i < 3; i++) { glBindBuffer(0x8892, bigVertex[i]); glBufferSubData(0x8892, 0, sizeof(bigStreams[i]), bigStreams[i]); }
        }
        uploaded += now() - uploadStart;
        double start = now();
        for (int d = 0; d < DRAWS; d++) {
            glUniform1ui(loc, d);
            const int count = big ? 24 : 6;
            const uintptr_t offset = big ? (uintptr_t)d * 24 * (wide ? 4 : 2) : 0;
            const unsigned type = wide ? 0x1405 : 0x1403;
            switch (call) {
            case Plain: glDrawElements(0x0004, count, type, (const void *)offset); break;
            case InstancedBase: glDrawElementsInstancedBaseVertex(0x0004, count, type, (const void *)offset, 1, 0); break;
            case Ranged: glDrawRangeElements(0x0004, big ? d * 16 : 0, big ? (d * 16 + 15) : 3, count, type, (const void *)offset); break;
            }
        }
        spent += now() - start;
        glFlush();
        glFinish();
    }
    printf("%-46s %7.1f us per draw, upload %7.0f us per frame, error 0x%x\n", label, spent / (DRAWS * FRAMES), uploaded / FRAMES, glGetError());
    fflush(stdout);
}

int main(int argc, char **argv) {
    const char *library = (argc > 1) ? argv[1] : "libmali.so.1";
    void *lib = dlopen(library, RTLD_NOW | RTLD_GLOBAL);
    if (lib == NULL) { printf("no %s: %s\n", library, dlerror()); return 2; }
    void *glLib = lib;
    if (argc > 2) { glLib = dlopen(argv[2], RTLD_NOW | RTLD_GLOBAL); if (glLib == NULL) { printf("no %s\n", argv[2]); return 2; } }
#define E(name) name = (name##_t)dlsym(lib, #name); if (name == NULL) { printf("missing %s\n", #name); return 2; }
#define G(name) name = (name##_t)dlsym(glLib, #name); if (name == NULL) { printf("missing %s\n", #name); return 2; }
    E(eglGetDisplay) E(eglInitialize) E(eglBindAPI) E(eglChooseConfig) E(eglCreateContext) E(eglCreatePbufferSurface) E(eglMakeCurrent) E(eglGetError)
    P display = eglGetDisplay(NULL);
    int major = 0, minor = 0;
    if (!eglInitialize(display, &major, &minor)) { printf("eglInitialize failed 0x%x\n", eglGetError()); return 2; }
    eglBindAPI(0x30A0);
    const int configAttribs[] = { 0x3033, 0x0001, 0x3040, 0x0040, 0x3038 };
    P config = NULL;
    int count = 0;
    eglChooseConfig(display, configAttribs, &config, 1, &count);
    if (count == 0) { printf("no config 0x%x\n", eglGetError()); return 2; }
    const int contextAttribs[] = { 0x3098, 3, 0x30FB, 1, 0x3038 };
    P context = eglCreateContext(display, config, NULL, contextAttribs);
    const int surfaceAttribs[] = { 0x3057, 16, 0x3056, 16, 0x3038 };
    P surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
    if (!eglMakeCurrent(display, surface, surface, context)) { printf("makecurrent failed 0x%x\n", eglGetError()); return 2; }
    G(glCreateShader) G(glShaderSource) G(glCompileShader) G(glGetShaderiv) G(glGetShaderInfoLog) G(glCreateProgram) G(glAttachShader) G(glLinkProgram)
    G(glGetProgramiv) G(glGetProgramInfoLog) G(glUseProgram) G(glGetUniformLocation) G(glUniform1ui) G(glUniform1i) G(glUniform2f) G(glGenTextures) G(glBindTexture)
    G(glActiveTexture) G(glTexStorage2D) G(glTexSubImage2D) G(glTexParameteri) G(glGenFramebuffers) G(glBindFramebuffer) G(glFramebufferTexture2D) G(glGenBuffers)
    G(glBindBuffer) G(glBufferData) G(glBindBufferBase) G(glBindBufferRange) G(glGenVertexArrays) G(glBindVertexArray) G(glEnableVertexAttribArray)
    G(glVertexAttribPointer) G(glDrawElements) G(glViewport) G(glScissor) G(glEnable) G(glDisable) G(glBlendFuncSeparate) G(glDepthFunc) G(glDepthMask) G(glClear)
    G(glFlush) G(glFinish) G(glGetError) G(glGetString)
    printf("%s\n", glGetString(0x1F02));

    static uint32_t pixels[64];
    for (int i = 0; i < 64; i++) { pixels[i] = 0xFF000000u | (i * 0x030507u); }
    glGenTextures(64, textures);
    for (int i = 0; i < 64; i++) {
        glBindTexture(0x0DE1, textures[i]);
        glTexStorage2D(0x0DE1, 1, 0x8058, 8, 8);
        glTexSubImage2D(0x0DE1, 0, 0, 0, 8, 8, 0x1908, 0x1401, pixels);
        glTexParameteri(0x0DE1, 0x2801, 0x2600);
        glTexParameteri(0x0DE1, 0x2800, 0x2600);
    }

    unsigned color = 0, depth = 0;
    glGenTextures(1, &color);
    glBindTexture(0x0DE1, color);
    glTexStorage2D(0x0DE1, 1, 0x8058, 304, 228);
    glGenTextures(1, &depth);
    glBindTexture(0x0DE1, depth);
    glTexStorage2D(0x0DE1, 1, 0x8CAC, 304, 228);
    glGenFramebuffers(2, framebuffers);
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(0x8D40, framebuffers[i]);
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
        glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depth, 0);
    }
    glViewport(0, 0, 304, 228);

    static float storage[4096];
    unsigned buffers[6];
    glGenBuffers(6, buffers);
    ssbo[0] = buffers[0]; ssbo[1] = buffers[1]; ubo[0] = buffers[2]; ubo[1] = buffers[3];
    for (int i = 0; i < 4; i++) {
        glBindBuffer(0x90D2, buffers[i]);
        glBufferData(0x90D2, sizeof(storage), storage, 0x88E4);
    }
    for (unsigned i = 0; i < 4; i++) { glBindBufferBase(0x90D2, i, ssbo[i & 1]); }
    glBindBufferBase(0x8A11, 0, ubo[0]);
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    static const float vertices[] = { -0.02f, -0.02f, 0.02f, -0.02f, 0.02f, 0.02f, -0.02f, 0.02f };
    static const uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    glBindBuffer(0x8892, buffers[4]);
    glBufferData(0x8892, sizeof(vertices), vertices, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, 0);
    glBindBuffer(0x8893, buffers[5]);
    glBufferData(0x8893, sizeof(indices), indices, 0x88E4);

    programs[0] = makeProgram(16);
    programs[1] = makeProgram(16);
    programs[2] = makeProgram(2);
    for (int i = 0; i < 3; i++) {
        locIndex[i] = glGetUniformLocation(programs[i], "renderIndex");
        locOffset[i] = glGetUniformLocation(programs[i], "offset");
    }

    glDrawElementsInstancedBaseVertex = (glDrawElementsInstancedBaseVertex_t)dlsym(glLib, "glDrawElementsInstancedBaseVertex");
    G(glDrawRangeElements) G(glBufferSubData)
    vaoOne = vao;
    currentVertexSource = vertexSource3;
    program3 = makeProgram(16);
    currentVertexSource = NULL;
    glGenVertexArrays(1, &vaoThree);
    glBindVertexArray(vaoThree);
    glGenBuffers(3, bigVertex);
    for (int v = 0; v < DRAWS * 16; v++) {
        const float x = (float)(v & 3) * 0.01f, y = (float)((v >> 2) & 3) * 0.01f;
        bigStreams[0][v * 4] = x - 0.02f; bigStreams[0][v * 4 + 1] = y - 0.02f;
        bigStreams[1][v * 4] = x; bigStreams[1][v * 4 + 1] = y;
        bigStreams[2][v * 4] = 1.0f; bigStreams[2][v * 4 + 3] = 1.0f;
    }
    for (int i = 0; i < 3; i++) {
        glBindBuffer(0x8892, bigVertex[i]);
        glBufferData(0x8892, sizeof(bigStreams[i]), bigStreams[i], 0x88E8);
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, 4, 0x1406, 0, 16, 0);
    }
    for (int d = 0; d < DRAWS; d++) {
        for (int i = 0; i < 24; i++) {
            bigIndices32[d * 24 + i] = (uint32_t)(d * 16 + ((i % 3) + (i / 3)) % 16);
            bigIndices16[d * 24 + i] = (uint16_t)bigIndices32[d * 24 + i];
        }
    }
    unsigned bigBuffers[2];
    glGenBuffers(2, bigBuffers);
    bigIndex16 = bigBuffers[0];
    bigIndex32 = bigBuffers[1];
    glBindBuffer(0x8893, bigIndex16);
    glBufferData(0x8893, sizeof(bigIndices16), bigIndices16, 0x88E8);
    glBindBuffer(0x8893, bigIndex32);
    glBufferData(0x8893, sizeof(bigIndices32), bigIndices32, 0x88E8);
    glBindVertexArray(vaoOne);
    glBindBuffer(0x8893, buffers[5]);

    printf("-- every draw also sets one uniform\n");
    runDrawKind("baseline: glDrawElements, 6 shared indices", Plain, 0, 0, 0, 0, 0);
    runDrawKind("instanced base vertex, 6 shared indices", InstancedBase, 0, 0, 0, 0, 0);
    runDrawKind("range, 6 shared indices", Ranged, 0, 0, 0, 0, 0);
    glBindVertexArray(vaoThree);
    glBindBuffer(0x8893, buffers[5]);
    runDrawKind("3 attributes, 6 shared indices", Plain, 1, 0, 0, 0, 0);
    runDrawKind("3 attributes, 24 own indices 16 bit", Plain, 1, 0, 1, 0, 0);
    runDrawKind("3 attributes, 24 own indices 32 bit", Plain, 1, 1, 1, 0, 0);
    runDrawKind("same, indices uploaded each frame", Plain, 1, 1, 1, 1, 0);
    runDrawKind("same, instanced base vertex", InstancedBase, 1, 1, 1, 1, 0);
    runDrawKind("same, range", Ranged, 1, 1, 1, 1, 0);
    runDrawKind("same, 16 bit, instanced base vertex", InstancedBase, 1, 0, 1, 1, 0);
    runDrawKind("instanced, 32 bit, indices and vertices uploaded", InstancedBase, 1, 1, 1, 1, 1);
    runDrawKind("range, 32 bit, indices and vertices uploaded", Ranged, 1, 1, 1, 1, 1);
    return 0;
}
