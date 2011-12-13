#ifndef _LSD_DATA_CONTAINER_HPP_INCLUDED_
#define _LSD_DATA_CONTAINER_HPP_INCLUDED_

#include <string>
#include <stdexcept>
#include <sys/time.h>

#include <boost/shared_ptr.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/thread/mutex.hpp>

#include "structs.hpp"

namespace lsd {

class data_container {

public:
	data_container();
	data_container(const void* data, size_t size);
	explicit data_container(const data_container& dc);
	virtual ~data_container();
	
	data_container& operator = (const data_container& rhs);
	bool operator == (const data_container& rhs) const;
	bool operator != (const data_container& rhs) const;

	void* data() const;
	size_t size() const;
	bool empty() const;
	void clear();

private:
	// sha1 size in bytes
	static const size_t SHA1_SIZE = 20;

	// sha1-encoded data chunk size - 512 kb
	static const size_t SHA1_CHUNK_SIZE = 512 * 1024;

	// max amount of data that does not need sha1 signature 1 mb
	static const size_t SMALL_DATA_SIZE = 1024 * 1024;

	typedef boost::detail::atomic_count reference_counter;
	
	void init();
	void init_with_data(unsigned char* data, size_t size);
	void swap(data_container& other);
	void sign_data(unsigned char* data, size_t& size, unsigned char signature[SHA1_SIZE]);

private:
	// data
	unsigned char* data_;
	size_t size_;

	// data sha1 signature
	bool signed_;
	unsigned char signature_[SHA1_SIZE];

	// data reference counter
	boost::shared_ptr<reference_counter> ref_counter_;

	// synchronization
	boost::mutex mutex_;
};

} // namespace lsd

#endif // _LSD_DATA_CONTAINER_HPP_INCLUDED_
