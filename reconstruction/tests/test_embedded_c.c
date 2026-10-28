#ifdef NDEBUG
#undef NDEBUG
#endif
#include "resources.h"
#include "embedded.h"
#include "assets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compare the compiled result with an independent external C decode, including
 * cross-sprite byte planes used by the original raster and core world tables. */
int main(int argc,char **argv)
{
    const char *names[]={"NOMPERS","VALMON","VERBES","OBJETS","NOMON","MESSAGE","MESSAGE2","MESSAGE3",
        "CRAPIM.IMA","ASTRON.IMA","BANNIERE.IMA","FACADE.IMA","KRONO.IMA","MAISONJ.IMA","MAISONS.IMA"};
    char error[512];unsigned i;TeraResources *embedded,*external;
    TeraExplorationState *a=calloc(1,sizeof *a),*b=calloc(1,sizeof *b);
    assert(a && b);
    embedded=tera_resources_load(NULL,error,sizeof error);assert(embedded);
    assert(embedded->atlas.used==92168);
    assert(tera_assets_world(a,NULL,1986,0,error,sizeof error));
    if(argc>1) {
        char path[4096];FILE *probe;
        snprintf(path,sizeof path,"%s/NOMPERS",argv[1]);probe=fopen(path,"rb");
        if(!probe) {
            free(a);free(b);free(embedded);
            puts("SKIP: external comparison data absent (embedded startup passed)");return 77;
        }
        fclose(probe);
        for(i=0;i<sizeof names/sizeof names[0];++i) {
            size_t size;const unsigned char *bytes=tera_embedded_resource(names[i],&size);
            unsigned char *file=malloc(size);assert(file && bytes);
            assert(tera_resource_read(argv[1],names[i],file,size,error,sizeof error));
            assert(!memcmp(bytes,file,size));free(file);
        }
        external=tera_resources_load(argv[1],error,sizeof error);assert(external);
        assert(!memcmp(embedded,external,sizeof *embedded));free(external);
        assert(tera_assets_world(b,argv[1],1986,0,error,sizeof error));
        assert(!memcmp(a,b,sizeof *a));
        puts("PASS: 15 embedded byte arrays, all decoded text/pixels/descriptors/planes and generated world identical to external C loaders");
    }
    assert(!tera_resources_load("missing-explicit-development-data",error,sizeof error));
    free(a);free(b);free(embedded);
    puts("PASS: embedded startup without originals; explicit missing override rejected");return 0;
}
