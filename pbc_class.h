#ifndef _PBC_CLASS__
#define _PBC_CLASS__

#include "core/reference.h"

extern "C" {
struct pbc_env;
struct pbc_rmessage;
struct pbc_wmessage;
}

class PBCRMsg: public Reference {
    GDCLASS(PBCRMsg, Reference);

    struct pbc_rmessage *_msg;
    bool _isRoot;

protected:
    static void _bind_methods();

public:
    inline PBCRMsg(struct pbc_rmessage *m = nullptr, bool r = false): _msg(m), _isRoot(r) {}
    ~PBCRMsg();

    size_t getSize(const String &key);
    int64_t getInt(const String &key, int index = 0);
    uint64_t getUInt(const String &key, int index = 0);
    double getReal(const String &key, int index = 0);
    String getString(const String &key, int index = 0);
    Ref<PBCRMsg> getMsg(const String &key, int index = 0);
};

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
