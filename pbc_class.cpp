#include "pbc_class.h"

#include "pbc.h"

#include "resource.h"
#include "os/file_access.h"

#include <stdio.h>
#include <stdlib.h>

void PBCRMsg::_bind_methods() {
    ObjectTypeDB::bind_method("get_size", &PBCRMsg::getSize);
    ObjectTypeDB::bind_method("get_int", &PBCRMsg::getInt);
    ObjectTypeDB::bind_method("get_uint", &PBCRMsg::getUInt);
    ObjectTypeDB::bind_method("get_real", &PBCRMsg::getReal);
    ObjectTypeDB::bind_method("get_string", &PBCRMsg::getString);
    ObjectTypeDB::bind_method("get_msg", &PBCRMsg::getMsg);
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
    ObjectTypeDB::bind_method("set_int", &PBCWMsg::setInt);
    ObjectTypeDB::bind_method("set_uint", &PBCWMsg::setUInt);
    ObjectTypeDB::bind_method("set_real", &PBCWMsg::setReal);
    ObjectTypeDB::bind_method("set_string", &PBCWMsg::setString);
    ObjectTypeDB::bind_method("mutable_msg", &PBCWMsg::mutableMsg);
    ObjectTypeDB::bind_method("encode", &PBCWMsg::encode);
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
    DVector<uint8_t> *vec = new DVector<uint8_t>;
    vec->resize(slice.len);
    memcpy(vec->write().ptr(), slice.buffer, slice.len);
    return *vec;
}

void PBCEnv::_bind_methods() {
    ObjectTypeDB::bind_method("register_proto", &PBCEnv::registerProto);
    ObjectTypeDB::bind_method("decode", &PBCEnv::decode);
    ObjectTypeDB::bind_method("new_msg", &PBCEnv::newMsg);
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

Ref<PBCRMsg> PBCEnv::decode(const String &type, const DVector<uint8_t> &var) {
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