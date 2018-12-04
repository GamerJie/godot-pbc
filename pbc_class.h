#ifndef _PBC_CLASS__
#define _PBC_CLASS__

#include "reference.h"

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

class PBCWMsg: public Reference {
    GDCLASS(PBCWMsg, Reference);

    struct pbc_wmessage *_msg;
    bool _isRoot;

protected:
    static void _bind_methods();

public:
    inline PBCWMsg(struct pbc_wmessage *m = nullptr, bool r = false): _msg(m), _isRoot(r) {}
    ~PBCWMsg();

    void setInt(const String &key, int64_t val);
    void setUInt(const String &key, uint64_t val);
    void setReal(const String &key, double val);
    void setString(const String &key, const String &val);
    Ref<PBCWMsg> mutableMsg(const String &key);

    Variant encode();
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
    Ref<PBCRMsg> decode(const String &type, const PoolVector<uint8_t> &var);
    Ref<PBCWMsg> newMsg(const String &type);
};

#endif // _PBC_CLASS__
