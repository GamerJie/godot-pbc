#include "pbc_class.h"

#include "pbc.h"

#include "resource.h"
#include "os/file_access.h"

#include <stdio.h>
#include <stdlib.h>

void PBCRMsg::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_size", "field"), &PBCRMsg::getSize);
    ClassDB::bind_method(D_METHOD("get_int", "field", "index"), &PBCRMsg::getInt, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_uint", "field", "index"), &PBCRMsg::getUInt, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_real", "field", "index"), &PBCRMsg::getReal, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_string", "field", "index"), &PBCRMsg::getString, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_msg", "field", "index"), &PBCRMsg::getMsg, DEFVAL(0));
}

PBCRMsg::~PBCRMsg() {
    if (_msg != nullptr && _isRoot) {
        pbc_rmessage_delete(_msg);
    }
}

size_t PBCRMsg::getSize(const String &key) {
    return pbc_rmessage_size(_msg, key.utf8().get_data());
}

int64_t PBCRMsg::getInt(const String &key, int index) {
    return (int64_t)getUInt(key, index);
}

uint64_t PBCRMsg::getUInt(const String &key, int index) {
    uint32_t hi;
    uint32_t lo = pbc_rmessage_integer(_msg, key.utf8().get_data(), index, &hi);
    return (uint64_t)lo | ((uint64_t)hi << 32);
}

double PBCRMsg::getReal(const String &key, int index) {
    return pbc_rmessage_real(_msg, key.utf8().get_data(), index);
}

String PBCRMsg::getString(const String &key, int index) {
    const char *str = pbc_rmessage_string(_msg, key.utf8().get_data(), index, nullptr);
    return String(str);
}

Ref<PBCRMsg> PBCRMsg::getMsg(const String &key, int index) {
    struct pbc_rmessage *msg = pbc_rmessage_message(_msg, key.utf8().get_data(), index);
    if (msg == nullptr) return nullptr;
    PBCRMsg *rmsg = new PBCRMsg(msg, false);
    return rmsg;
}

void PBCWMsg::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_int", "field", "value"), &PBCWMsg::setInt);
    ClassDB::bind_method(D_METHOD("set_uint", "field", "value"), &PBCWMsg::setUInt);
    ClassDB::bind_method(D_METHOD("set_real", "field", "value"), &PBCWMsg::setReal);
    ClassDB::bind_method(D_METHOD("set_string", "field", "value"), &PBCWMsg::setString);
    ClassDB::bind_method(D_METHOD("mutable_msg", "field"), &PBCWMsg::mutableMsg);
    ClassDB::bind_method(D_METHOD("encode"), &PBCWMsg::encode);
}

PBCWMsg::~PBCWMsg() {
    if (_msg != nullptr && _isRoot) {
        pbc_wmessage_delete(_msg);
    }
}

void PBCWMsg::setInt(const String &key, int64_t val) {
    setUInt(key, (uint64_t)val);
}

void PBCWMsg::setUInt(const String &key, uint64_t val) {
    pbc_wmessage_integer(_msg, key.utf8().get_data(), static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32));
}

void PBCWMsg::setReal(const String &key, double val) {
    pbc_wmessage_real(_msg, key.utf8().get_data(), val);
}

void PBCWMsg::setString(const String &key, const String &val) {
    const CharString &cstr = val.utf8();
    pbc_wmessage_string(_msg, key.utf8().get_data(), cstr.get_data(), cstr.length());
}

Ref<PBCWMsg> PBCWMsg::mutableMsg(const String &key) {
    struct pbc_wmessage *msg = pbc_wmessage_message(_msg, key.utf8().get_data());
    if (msg == nullptr) return nullptr;
    PBCWMsg *wmsg = new PBCWMsg(msg, false);
    return wmsg;
}

Variant PBCWMsg::encode() {
    struct pbc_slice slice;
    pbc_wmessage_buffer(_msg, &slice);
    PoolVector<uint8_t> *vec = new PoolVector<uint8_t>;
    vec->resize(slice.len);
    memcpy(vec->write().ptr(), slice.buffer, slice.len);
    return *vec;
}

void PBCEnv::_bind_methods() {
    ClassDB::bind_method(D_METHOD("register_proto", "filename"), &PBCEnv::registerProto);
    ClassDB::bind_method(D_METHOD("decode", "type", "data"), &PBCEnv::decode);
    ClassDB::bind_method(D_METHOD("new_msg", "type"), &PBCEnv::newMsg);
}

PBCEnv::PBCEnv(): _env(pbc_new()) { }

PBCEnv::~PBCEnv() {
    pbc_delete(_env);
}

bool PBCEnv::registerProto(const String &filename) {
    FileAccess *fa = FileAccess::open(filename, FileAccess::READ);
    if (fa == nullptr) return false;
    struct pbc_slice slice;
    size_t len = fa->get_len();
    slice.len = len;
    slice.buffer = malloc(len);
    fa->get_buffer(static_cast<uint8_t*>(slice.buffer), len);
    pbc_register(_env, &slice);
    free(slice.buffer);
    memdelete(fa);
}

Ref<PBCRMsg> PBCEnv::decode(const String &type, const PoolVector<uint8_t> &var) {
    struct pbc_slice slice;
    slice.len = var.size();
    slice.buffer = (void*)var.read().ptr();
    struct pbc_rmessage *msg = pbc_rmessage_new(_env, type.utf8().get_data(), &slice);
    if (msg == nullptr) return nullptr;
    PBCRMsg *rmsg = new PBCRMsg(msg, true);
    return rmsg;
}

Ref<PBCWMsg> PBCEnv::newMsg(const String &type) {
    struct pbc_wmessage *msg = pbc_wmessage_new(_env, type.utf8().get_data());
    if (msg == nullptr) return nullptr;
    PBCWMsg *wmsg = new PBCWMsg(msg, true);
    return wmsg;
}
