#include "settings.h"

#include <cstring>
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>
#include <boost/shared_ptr.hpp>

#include <uuid/uuid.h>

#include <openssl/sha.h>
#include <openssl/evp.h>

#include "json/json.h"
#include "structs.hpp"

#include "data_container.hpp"

namespace lsd {

data_container::data_container() :
	data_(NULL),
	size_(0),
	signed_(false)
{
	init();
}

data_container::data_container(unsigned char* data, size_t size) :
	data_(data),
	size_(size),
	signed_(false)
{
	init();

	if (data == NULL || size == 0) {
		data_ = NULL;
		size_ = 0;
		return;
	}

	std::string error_msg = "not enough memory to create new data container at ";
	error_msg += std::string(BOOST_CURRENT_FUNCTION);

	// allocate new space
	try {
		data_ = new unsigned char[size];
	}
	catch (...) {
		throw std::runtime_error(error_msg);
	}

	if (!data_) {
		throw std::runtime_error(error_msg);
	}

	// copy data
	memcpy(data_, data, size);
	++*ref_counter_;

	if (size <= SMALL_DATA_SIZE) {
		return;
	}

	sign_data(data_, size_, signature_);
	signed_ = true;
}

data_container::data_container(const data_container& dc) {
	if (dc.empty()) {
		init();
		return;
	}

	data_ = dc.data_;
	size_ = dc.size_;

	if (dc.signed_) {
		memcpy(signature_, dc.signature_, SHA1_SIZE);
		signed_ = dc.signed_;
	}

	ref_counter_ = dc.ref_counter_;
	++*ref_counter_;
}

void
data_container::init() {
	// reset sha1 signature
	signed_ = false;
	memset(signature_, 0, SHA1_SIZE);

	// create ref counter
	std::string error_msg = "not enough memory to create new ref_counter in data container at ";
	error_msg += std::string(BOOST_CURRENT_FUNCTION);

	try {
		ref_counter_.reset(new reference_counter(0));
	}
	catch (...) {
		throw std::runtime_error(error_msg);
	}

	if (!ref_counter_.get()) {
		throw std::runtime_error(error_msg);
	}

	// init data
	data_ = NULL;
	size_ = 0;
}

data_container::~data_container() {
	--*ref_counter_;

	if (data_ && *ref_counter_ == 0) {
		delete [] data_;
	}
}

data_container&
data_container::operator = (const data_container &rhs) {
	data_container(rhs).swap(*this);
	return *this;
}

bool
data_container::operator == (const data_container &rhs) const {
	// data size differs?
	if (size_ != rhs.size_) {
		return false;
	}

	// both containers empty?
	if (size_ == 0 && rhs.size_ == 0) {
		return true;
	}

	// compare small containers
	if (size_ <= SMALL_DATA_SIZE) {
		return (0 == memcmp(data_, rhs.data_, size_));
	}

	// compare big containers
	return (0 == memcmp(signature_, rhs.signature_, SHA1_SIZE));
}

bool
data_container::operator != (const data_container &rhs) const {
	return !(*this == rhs);
}

bool
data_container::empty() const {
	return (size_ == 0);
}

void
data_container::clear() {
	--*ref_counter_;

	if (data_ && *ref_counter_ == 0) {
		delete data_;
	}

	init();
}

unsigned char*
data_container::data() const {
	return data_;
}

size_t
data_container::size() const {
	return size_;
}

void
data_container::swap(data_container& other) {
	std::swap(data_, other.data_);
	std::swap(size_, other.size_);
	std::swap(signed_, other.signed_);

	unsigned char signature_tmp[SHA1_SIZE];
	memcpy(signature_tmp, signature_, SHA1_SIZE);
	memcpy(signature_, other.signature_, SHA1_SIZE);
	memcpy(other.signature_, signature_tmp, SHA1_SIZE);

	std::swap(ref_counter_, other.ref_counter_);
}

void
data_container::sign_data(unsigned char* data, size_t& size, unsigned char signature[SHA1_SIZE]) {
	SHA_CTX sha_context;
	SHA1_Init(&sha_context);

	// reset signature
	memset(signature, 0, sizeof(signature));

	// go through compiled and flattened data in chunks of 512 kb., building signature
	unsigned int full_chunks_count = size / SHA1_CHUNK_SIZE;
	char* data_offset_ptr = (char*)data;

	// process full chunks
	for (unsigned i = 0; i < full_chunks_count; i++) {
		SHA1_Update(&sha_context, data_offset_ptr, SHA1_CHUNK_SIZE);
		data_offset_ptr += SHA1_CHUNK_SIZE;
	}

	unsigned int remainder_chunk_size = 0;

	if (size < SHA1_CHUNK_SIZE) {
		remainder_chunk_size = size;
	}
	else {
		remainder_chunk_size = size % SHA1_CHUNK_SIZE;
	}

	// process remained chunk
	if (0 != remainder_chunk_size) {
		SHA1_Update(&sha_context, data_offset_ptr, remainder_chunk_size);
	}

	SHA1_Final(signature, &sha_context);
}

} // namespace lsd
