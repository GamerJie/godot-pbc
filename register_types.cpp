#include "register_types.h"

#include "pbc_class.h"

void register_pbc_types() {
    ObjectTypeDB::register_type<PBCRMsg>();
    ObjectTypeDB::register_type<PBCWMsg>();
    ObjectTypeDB::register_type<PBCEnv>();
}

void unregister_pbc_types() {
}
