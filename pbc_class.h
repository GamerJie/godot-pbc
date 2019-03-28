#ifndef _PBC_CLASS__
#define _PBC_CLASS__

#include "core/reference.h"

extern "C" {
struct pbc_env;
struct pbc_rmessage;
struct pbc_wmessage;
}

class PBCEnv: public Reference {
    GDCLASS(PBCEnv, Reference);

    struct pbc_env *_env;

protected:
    static void _bind_methods();

public:
    PBCEnv();
    ~PBCEnv();
    bool registerProto(const String &filename);
	PoolByteArray encode(const String &type, Dictionary dict);
	Dictionary decode(const String& type, PoolByteArray data);
	int enumId(const String &type, const String &name);
};

#endif // _PBC_CLASS__
