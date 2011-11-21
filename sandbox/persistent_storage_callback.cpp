#include "settings.h"

#include "persistent_storage_callback.hpp"

namespace lsd {

persistent_storage_callback::persistent_storage_callback() {
	
}

persistent_storage_callback::persistent_storage_callback(callback_function callback) {
	callback_ = callback;
}

persistent_storage_callback::~persistent_storage_callback() {	
}

bool
persistent_storage_callback::callback(const zbr::eblob_disk_control* dco, const void* data, int) {
	if ((dco->flags & BLOB_DISK_CTL_REMOVE) == BLOB_DISK_CTL_REMOVE)  {
		return true;
	}
	
	std::string value((char*)data, dco->data_size);
	
	if (callback_) {
		callback_(value);
	}

	return true;
}

void
persistent_storage_callback::complete(uint64_t, uint64_t) {
	
}

} // namespace lsd