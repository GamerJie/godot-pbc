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
    if (fa == nullptr)
		return false;

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

struct pbc_wmessage * encode_dict(struct pbc_env * _env, const String &type, Dictionary dict, pbc_wmessage *parent = nullptr) {
	struct pbc_wmessage *msg;
	if(parent == nullptr)
		msg = pbc_wmessage_new(_env, type.utf8().get_data());
	else
		msg = pbc_wmessage_message(parent , type.utf8().get_data());
	
	if (msg == nullptr)
		return nullptr;

	List<Variant> keys;
	dict.get_key_list(&keys);
	for (List<Variant>::Element *E = keys.front(); E; E = E->next()) {
		String key = String(E->get());
		Variant value = dict[E->get()];

		if (value.get_type() == Variant::INT) {
			uint64_t val = static_cast<uint64_t>(value);
			pbc_wmessage_integer(msg, key.utf8().get_data(), static_cast<uint32_t>(val), 0);
		} else if (value.get_type() == Variant::REAL) {
			uint64_t val = static_cast<uint64_t>(value);
			pbc_wmessage_real(msg, key.utf8().get_data(), val);
		} else if (value.get_type() == Variant::STRING) {
			const CharString str = String(value).utf8();
			pbc_wmessage_string(msg, key.utf8().get_data(), str.get_data(), str.length());
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

	PoolByteArray data;
	data.resize(slice.len);
	memcpy(data.write().ptr(), slice.buffer, slice.len);
	pbc_wmessage_delete(msg);
	
	return data;
}

void decode_callback(void *ud, int type, const char *tname, union pbc_value *v, int id, const char *key) {
	struct pbc_godot_data *pbc_data = (struct pbc_godot_data *)ud;
	//Dictionary *dict = (Dictionary *)ud;
		
	switch(type & ~PBC_REPEATED) {
		case PBC_MESSAGE: {
			Dictionary second;
			struct pbc_godot_data senond_data;
			senond_data.env = pbc_data->env;
			senond_data.dict = &second;

			// printf("[%s]  -> \n" , tname);
			pbc_decode(pbc_data->env, tname, &(v->s), decode_callback, &senond_data);
			(*pbc_data->dict)[key] = second;
			// printf("---------\n");
			break;
		}
		case PBC_INT:
			// printf("%s : %d\n", key, (int)v->i.low);
			(*pbc_data->dict)[key] = v->i.low;
			break;
		case PBC_REAL:
			// printf("%lf\n", v->f);
			(*pbc_data->dict)[key] = v->f;
			break;
		case PBC_BOOL:
			// printf("<%s>\n", v->i.low ? "true" : "false");
			(*pbc_data->dict)[key] = v->i.low ? true : false;
			break;
		case PBC_ENUM:
			// printf("[%s:%d]\n", v->e.name , v->e.id);
			break;
		case PBC_STRING: {
			char *buffer = (char*)malloc((v->s.len + 1) * sizeof(char));
			memset(buffer, 0, v->s.len+1);
			memcpy(buffer, v->s.buffer, v->s.len);
			buffer[v->s.len] = '\0';
			//printf("%s : \"%s\"\n", key, buffer);

			(*pbc_data->dict)[key] = String::utf8(buffer);
			break;
		}
		case PBC_BYTES: {
			// int i;
			// uint8_t *buffer = (uint8_t*)v->s.buffer;
			// for (i=0;i<v->s.len;i++) {
				// printf("%02X ",buffer[i]);
			// }
			// printf("\n");
			break;
		}
		case PBC_INT64: {
			// printf("%s : %ld\n", key, v->i.low);
			(*pbc_data->dict)[key] = v->i.low;
			break;
		}
		case PBC_UINT:
			// printf("%u\n",v->i.low);
			(*pbc_data->dict)[key] = v->i.low;
			break;
		default:
			// printf("!!! %d\n", type);
			break;
	}
}

Dictionary PBCEnv::decode(const String &type, PoolByteArray data) {
	Dictionary dict;
	struct pbc_slice slice;
    slice.len = data.size();
	slice.buffer = malloc(slice.len);
	//(void *)data.read().ptr();
	memcpy(slice.buffer, data.read().ptr(), slice.len);
	
	struct pbc_godot_data pbc_data;
	pbc_data.env = _env;
	pbc_data.dict = &dict;
	pbc_decode(_env, type.utf8().get_data(), &slice, decode_callback, &pbc_data);

	return dict;
}

int PBCEnv::enumId(const String &type, const String &name) {
	return pbc_enum_id(_env, type.utf8().get_data(), name.utf8().get_data());
}
