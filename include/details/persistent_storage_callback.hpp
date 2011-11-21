#ifndef _PMQ_PERSISTENT_STORAGE_CALLBACK_HPP_INCLUDED_
#define _PMQ_PERSISTENT_STORAGE_CALLBACK_HPP_INCLUDED_

#include <string>

#include <eblob/eblob.hpp>
#include <boost/function.hpp>

namespace pmq {

class persistent_storage_callback : public zbr::eblob_iterator_callback {
public:
	typedef boost::function<void(const std::string&)> callback_function;
	persistent_storage_callback();
	persistent_storage_callback(callback_function callback);
	virtual ~persistent_storage_callback();
	
	bool callback(const zbr::eblob_disk_control* dco, const void* data, int);
	void complete(uint64_t, uint64_t);
	
private:
	callback_function callback_;
};

} // namespace pmq

#endif // _PMQ_PERSISTENT_STORAGE_CALLBACK_HPP_INCLUDED_