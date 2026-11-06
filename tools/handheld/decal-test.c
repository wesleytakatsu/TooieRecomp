// Does a draw see the depth written just before it? N64 decals sample the depth target of the same framebuffer in the shader, so each way of ending the pass
// is tried here without the game: write depth, end the pass some way, sample it into color, compare. Also the cost of one write-read cycle.
//   decal-test [EGL library, default libmali.so.1] [GLES library]
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
L(void, glUniform1f, int, float)
L(void, glUniform1i, int, int)
L(void, glGenTextures, int, unsigned *)
L(void, glBindTexture, unsigned, unsigned)
L(void, glActiveTexture, unsigned)
L(void, glTexStorage2D, unsigned, int, unsigned, int, int)
L(void, glTexParameteri, unsigned, unsigned, int)
L(void, glGenFramebuffers, int, unsigned *)
L(void, glBindFramebuffer, unsigned, unsigned)
L(void, glFramebufferTexture2D, unsigned, unsigned, unsigned, unsigned, int)
L(void, glGenVertexArrays, int, unsigned *)
L(void, glBindVertexArray, unsigned)
L(void, glDrawArrays, unsigned, int, int)
L(void, glViewport, int, int, int, int)
L(void, glScissor, int, int, int, int)
L(void, glEnable, unsigned)
L(void, glDisable, unsigned)
L(void, glDepthFunc, unsigned)
L(void, glDepthMask, unsigned char)
L(void, glClear, unsigned)
L(void, glClearDepthf, float)
L(void, glClearColor, float, float, float, float)
L(void, glFlush, void)
L(void, glFinish, void)
L(void, glMemoryBarrier, unsigned)
L(void, glReadPixels, int, int, int, int, unsigned, unsigned, void *)
L(unsigned, glGetError, void)
L(const unsigned char *, glGetString, unsigned)
L(const unsigned char *, glGetStringi, unsigned, unsigned)
L(void, glGetIntegerv, unsigned, int *)

static const char *vertexSource =
    "#version 310 es\n"
    "uniform highp float z;\n"
    "void main() { vec2 p = vec2((gl_VertexID & 1) * 4 - 1, (gl_VertexID & 2) * 2 - 1); gl_Position = vec4(p, z * 2.0 - 1.0, 1.0); }\n";
static const char *writeSource =
    "#version 310 es\nprecision highp float;\nlayout(location = 0) out vec4 color;\nvoid main() { color = vec4(0.0, 0.0, 1.0, 1.0); }\n";
static const char *readSource =
    "#version 310 es\nprecision highp float;\nlayout(location = 0) out vec4 color;\nuniform highp sampler2D depthTexture;\n"
    "void main() { color = vec4(texelFetch(depthTexture, ivec2(gl_FragCoord.xy), 0).r, 0.0, 0.0, 1.0); }\n";
static const char *readFetchSource =
    "#version 310 es\n#extension GL_ARM_shader_framebuffer_fetch_depth_stencil : require\nprecision highp float;\nlayout(location = 0) out vec4 color;\n"
    "void main() { color = vec4(gl_LastFragDepthARM, 0.0, 0.0, 1.0); }\n";

static unsigned compile(unsigned type, const char *text) {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &text, NULL);
    glCompileShader(shader);
    int status = 0;
    glGetShaderiv(shader, 0x8B81, &status);
    if (!status) { static char log[4096]; glGetShaderInfoLog(shader, sizeof(log), NULL, log); printf("compile failed: %s\n", log); exit(2); }
    return shader;
}

static unsigned makeProgram(const char *pixel) {
    unsigned program = glCreateProgram();
    glAttachShader(program, compile(0x8B31, vertexSource));
    glAttachShader(program, compile(0x8B30, pixel));
    glLinkProgram(program);
    int status = 0;
    glGetProgramiv(program, 0x8B82, &status);
    if (!status) { static char log[4096]; glGetProgramInfoLog(program, sizeof(log), NULL, log); printf("link failed: %s\n", log); exit(2); }
    return program;
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

enum { SIZE = 64, CYCLES = 4, FRAMES = 8, COST_CYCLES = 40, COST_FRAMES = 25 };
enum Method { Nothing, Flush, Finish, OtherFbo, OtherFboOneWay, FlushOtherFbo, FlushBarrier, SameFbo, ThroughZero, DepthFetch, Count };
static const char *names[] = {
    "nothing", "glFlush", "glFinish", "other FBO, both ways", "other FBO, write to read only", "glFlush + other FBO", "glFlush + glMemoryBarrier", "bind same FBO",
    "bind 0, then same FBO", "ARM depth fetch"
};

static unsigned framebuffers[2], writeProgram, readProgram, fetchProgram;
static unsigned dummyDepth, realDepth;
static int writeZ, readZ, fetchZ, current, rebindTexture;

static void endPass(enum Method method) {
    switch (method) {
    case Flush: glFlush(); break;
    case Finish: glFinish(); break;
    case OtherFbo: case OtherFboOneWay: current ^= 1; glBindFramebuffer(0x8D40, framebuffers[current]); break;
    case FlushOtherFbo: glFlush(); current ^= 1; glBindFramebuffer(0x8D40, framebuffers[current]); break;
    case FlushBarrier: glFlush(); glMemoryBarrier(0xFFFFFFFFu); break;
    case SameFbo: glBindFramebuffer(0x8D40, framebuffers[current]); break;
    case ThroughZero: glBindFramebuffer(0x8D40, 0); glBindFramebuffer(0x8D40, framebuffers[current]); break;
    default: break;
    }
}

// The depth write, then the read at the same place: what the renderer does around a decal.
static void cycle(enum Method method, float z, int x, int width) {
    glScissor(x, 0, width, SIZE);
    if (rebindTexture) { glBindTexture(0x0DE1, dummyDepth); }
    glDepthMask(1);
    glUseProgram(writeProgram);
    glUniform1f(writeZ, z);
    glDrawArrays(0x0004, 0, 3);
    endPass(method);
    if (rebindTexture) { glBindTexture(0x0DE1, realDepth); }
    glDepthMask(0);
    if (method == DepthFetch) {
        glUseProgram(fetchProgram);
        glUniform1f(fetchZ, z);
    }
    else {
        glUseProgram(readProgram);
        glUniform1f(readZ, z);
    }
    glDrawArrays(0x0004, 0, 3);
    if ((method == OtherFbo) || (method == FlushOtherFbo)) {
        current ^= 1;
        glBindFramebuffer(0x8D40, framebuffers[current]);
    }
}

static void beginFrame(void) {
    current = 0;
    glBindFramebuffer(0x8D40, framebuffers[0]);
    glDepthMask(1);
    glDisable(0x0C11);
    glClear(0x4100);
    glEnable(0x0C11);
}

static void run(enum Method method) {
    int right = 0, wrong = 0, first = -1, firstExpected = -1;
    for (int frame = 0; frame < FRAMES; frame++) {
        beginFrame();
        for (int c = 0; c < CYCLES; c++) {
            cycle(method, 0.1f + 0.1f * c + 0.03f * frame, c * (SIZE / CYCLES), SIZE / CYCLES);
        }
        glFinish();
        for (int c = 0; c < CYCLES; c++) {
            unsigned char pixel[4] = { 0 };
            glReadPixels(c * (SIZE / CYCLES) + 2, SIZE / 2, 1, 1, 0x1908, 0x1401, pixel);
            int expected = (int)((0.1f + 0.1f * c + 0.03f * frame) * 255.0f + 0.5f);
            if (abs((int)pixel[0] - expected) <= 2) {
                right++;
            }
            else {
                wrong++;
                if (first < 0) { first = pixel[0]; firstExpected = expected; }
            }
        }
    }

    double spent = 0;
    for (int frame = 0; frame < COST_FRAMES; frame++) {
        beginFrame();
        double start = now();
        for (int c = 0; c < COST_CYCLES; c++) {
            cycle(method, 0.5f, (c * 3) % (SIZE - 2), 2);
        }
        glFinish();
        spent += now() - start;
    }

    printf("%-32s right %3d wrong %3d", names[method], right, wrong);
    if (first >= 0) { printf(" (read %d, wanted %d)", first, firstExpected); }
    printf("  %7.1f us per cycle, error 0x%x\n", spent / (COST_FRAMES * COST_CYCLES), glGetError());
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
    G(glGetProgramiv) G(glGetProgramInfoLog) G(glUseProgram) G(glGetUniformLocation) G(glUniform1f) G(glUniform1i) G(glGenTextures) G(glBindTexture)
    G(glActiveTexture) G(glTexStorage2D) G(glTexParameteri) G(glGenFramebuffers) G(glBindFramebuffer) G(glFramebufferTexture2D) G(glGenVertexArrays)
    G(glBindVertexArray) G(glDrawArrays) G(glViewport) G(glScissor) G(glEnable) G(glDisable) G(glDepthFunc) G(glDepthMask) G(glClear) G(glClearDepthf)
    G(glClearColor) G(glFlush) G(glFinish) G(glMemoryBarrier) G(glReadPixels) G(glGetError) G(glGetString) G(glGetStringi) G(glGetIntegerv)
    printf("%s\n", glGetString(0x1F02));

    int extensions = 0, hasFetch = 0;
    glGetIntegerv(0x821D, &extensions);
    for (int i = 0; i < extensions; i++) {
        const char *name = (const char *)glGetStringi(0x1F03, i);
        if (strstr(name, "ARM") || strstr(name, "fetch") || strstr(name, "pixel_local") || strstr(name, "tile")) { printf("  %s\n", name); }
        if (strcmp(name, "GL_ARM_shader_framebuffer_fetch_depth_stencil") == 0) { hasFetch = 1; }
    }

    unsigned color = 0, depth = 0;
    glGenTextures(1, &color);
    glBindTexture(0x0DE1, color);
    glTexStorage2D(0x0DE1, 1, 0x8058, SIZE, SIZE);
    glGenTextures(1, &depth);
    glBindTexture(0x0DE1, depth);
    glTexStorage2D(0x0DE1, 1, 0x8CAC, SIZE, SIZE);
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);
    glGenFramebuffers(2, framebuffers);
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(0x8D40, framebuffers[i]);
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
        glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depth, 0);
    }
    glViewport(0, 0, SIZE, SIZE);
    glClearDepthf(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(0x0B71);
    glDepthFunc(getenv("LEQUAL") ? 0x0203 : 0x0207);
    rebindTexture = (getenv("REBIND") != NULL);
    realDepth = depth;
    glGenTextures(1, &dummyDepth);
    glBindTexture(0x0DE1, dummyDepth);
    glTexStorage2D(0x0DE1, 1, 0x8CAC, SIZE, SIZE);
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);

    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    writeProgram = makeProgram(writeSource);
    readProgram = makeProgram(readSource);
    writeZ = glGetUniformLocation(writeProgram, "z");
    readZ = glGetUniformLocation(readProgram, "z");
    glUseProgram(readProgram);
    glUniform1i(glGetUniformLocation(readProgram, "depthTexture"), 0);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, depth);
    if (hasFetch) {
        fetchProgram = makeProgram(readFetchSource);
        fetchZ = glGetUniformLocation(fetchProgram, "z");
    }

    for (int method = 0; method < Count; method++) {
        if ((method != DepthFetch) || hasFetch) {
            run((enum Method)method);
        }
    }
    run(Nothing);
    return 0;
}
