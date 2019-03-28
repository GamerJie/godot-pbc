#include "pbc_class.h"

#include "pbc.h"

#include "core/resource.h"
#include "core/os/file_access.h"

#include <stdio.h>
#include <stdlib.h>

void PBCEnv::_bind_methods() {
    ClassDB::bind_method(D_METHOD("register_proto", "filename"), &PBCEnv::registerProto);
    ClassDB::bind_method(D_METHOD("decode", "type", "data"), &PBCEnv::decode);
    ClassDB::bind_method(D_METHOD("encode", "type", "dict"), &PBCEnv::encode);
	ClassDB::bind_method(D_METHOD("enum_id", "type", "name"), &PBCEnv::enumId);
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
	return true;
}

// Ref<PBCRMsg> PBCEnv::decode(const String &type, const PoolVector<uint8_t> &var) {
    // struct pbc_slice slice;
    // slice.len = var.size();
    // slice.buffer = (void*)var.read().ptr();
    // struct pbc_rmessage *msg = pbc_rmessage_new(_env, type.utf8().get_data(), &slice);
    // if (msg == nullptr) return nullptr;
    // PBCRMsg *rmsg = new PBCRMsg(msg, true);
    // return rmsg;
// }

struct pbc_wmessage * encode_dict(struct pbc_env * _env, const String &type, Dictionary dict, pbc_wmessage *parent = nullptr) {
	struct pbc_wmessage *msg;
	if(parent == nullptr)
		msg = pbc_wmessage_new(_env, type.ascii().get_data());
	else
		msg = pbc_wmessage_message(parent , type.ascii().get_data());
	
	if (msg == nullptr)
		return nullptr;

	List<Variant> keys;
	dict.get_key_list(&keys);
	for (List<Variant>::Element *E = keys.front(); E; E = E->next()) {
		String key = String(E->get());
		Variant value = dict[E->get()];

		if (value.get_type() == Variant::INT) {
			uint64_t val = static_cast<uint64_t>(value);
			pbc_wmessage_integer(msg, key.ascii().get_data(), static_cast<uint32_t>(val), 0);
		} else if (value.get_type() == Variant::REAL) {
			uint64_t val = static_cast<uint64_t>(value);
			pbc_wmessage_real(msg, key.ascii().get_data(), val);
		} else if (value.get_type() == Variant::STRING) {
			const CharString str = String(value).ascii();
			pbc_wmessage_string(msg, key.ascii().get_data(), str.get_data(), str.length());
		} else if (value.get_type() == Variant::DICTIONARY) {
			encode_dict(_env, key, value, msg);
		}
	}
	
	return msg;
}

PoolByteArray PBCEnv::encode(const String &type, Dictionary dict) {
	struct pbc_wmessage *msg = encode_dict(_env, type, dict);
	if(msg == nullptr)
		return PoolByteArray();
	
	struct pbc_slice slice;
	pbc_wmessage_buffer(msg, &slice);

	uint8_t vec[8 * 1024];
	memset(vec, 0, slice.len);
	memcpy(vec, slice.buffer, slice.len);
	pbc_wmessage_delete(msg);

	PoolByteArray data;
	data.resize(slice.len);

	for (int i = 0; i < slice.len; i++)
		data.set(i, vec[i]);
	
	return data;
}

void decode_callback(void *ud, int type, const char *tname, union pbc_value *v, int id, const char *key) {
	switch(type & ~PBC_REPEATED) {
		case PBC_MESSAGE:
			printf("[%s]  -> \n" , tname);
			pbc_decode((struct pbc_env *)ud, tname, &(v->s), decode_callback, ud);
			printf("---------\n");
			break;
		case PBC_INT:
			printf("%s : %d\n", key, (int)v->i.low);
			break;
		case PBC_REAL:
			printf("%lf\n", v->f);
			break;
		case PBC_BOOL:
			printf("<%s>\n", v->i.low ? "true" : "false");
			break;
		case PBC_ENUM:
			printf("[%s:%d]\n", v->e.name , v->e.id);
			break;
		case PBC_STRING: {
			char buffer[v->s.len+1];
			memcpy(buffer, v->s.buffer, v->s.len);
			buffer[v->s.len] = '\0';
			printf("%s : \"%s\"\n", key, buffer);
			break;
		}
		case PBC_BYTES: {
			int i;
			uint8_t *buffer = (uint8_t*)v->s.buffer;
			for (i=0;i<v->s.len;i++) {
				printf("%02X ",buffer[i]);
			}
			printf("\n");
			break;
		}
		case PBC_INT64: {
			printf("0x%x%08x\n",v->i.hi, v->i.low);
			break;
		}
		case PBC_UINT:
			printf("%u\n",v->i.low);
			break;
		default:
			printf("!!! %d\n", type);
			break;
	}
}

Dictionary PBCEnv::decode(const String& type, PoolByteArray data) {
	struct pbc_slice slice;
    slice.len = data.size();
    slice.buffer = (void*)data.read().ptr();
	struct pbc_rmessage * msg = pbc_rmessage_new(_env, type.ascii().get_data(), &slice);
	Dictionary dict;
	if(msg == nullptr)
		return dict;
	
	pbc_decode(_env, type.ascii().get_data(), &slice, decode_callback , _env);
	return dict;
}

int PBCEnv::enumId(const String &type, const String &name) {
	return pbc_enum_id(_env, type.utf8().get_data(), name.utf8().get_data());
}