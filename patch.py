m = open('main.c').read()
if 'max_vertices_floyd' in m:
    print('patch ja aplicado'); raise SystemExit
m = m.replace('#define MAX_VERTICES_FLOYD 30000',
 'static int max_vertices_floyd(void);\n#define MAX_VERTICES_FLOYD max_vertices_floyd()')
m = m.replace('static double para_gb(double bytes) { return bytes / (1024.0 * 1024.0 * 1024.0); }',
 '''static double para_gb(double bytes) { return bytes / (1024.0 * 1024.0 * 1024.0); }

static int max_vertices_floyd(void) {
    const char *env = getenv("TP3_MAX_V_FLOYD");
    if (env) {
        int v = atoi(env);
        if (v > 0) return v;
    }
    return 30000;
}''',1)
open('main.c','w').write(m)
print('patch aplicado')
