#pragma once

#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackWriter : public shv::chainpack::AbstractStreamWriter
{
	using Super = AbstractStreamWriter;
public:
	ChainPackWriter(std::ostream &out) : Super(out) {}

	ChainPackWriter& operator <<(const RpcValue &value) {write(value); return *this;}
	ChainPackWriter& operator <<(const RpcValue::MetaData &meta_data) {write(meta_data); return *this;}

	void write(const RpcValue &val) override;
	void write(const RpcValue::MetaData &meta_data) override;

	void writeUIntData(uint64_t n);
	//static void writeUIntData(std::ostream &os, uint64_t n);

	void writeContainerBegin(RpcValue::Type container_type) override;
	/// ChainPack doesn't need to know container type to close it
	void writeContainerEnd() override;
	void writeMapKey(const std::string &key) override;
	void writeIMapKey(RpcValue::Int key) override;
	void writeListElement(const RpcValue &val) override;
	void writeMapElement(const std::string &key, const RpcValue &val) override;
	void writeMapElement(RpcValue::Int key, const RpcValue &val) override;
	void writeRawData(const std::string &data) override;
private:
	ChainPackWriter& write_p(std::nullptr_t);
	ChainPackWriter& write_p(bool value);
	ChainPackWriter& write_p(int32_t value);
	ChainPackWriter& write_p(uint32_t value);
	ChainPackWriter& write_p(int64_t value);
	ChainPackWriter& write_p(uint64_t value);
	ChainPackWriter& write_p(double value);
	ChainPackWriter& write_p(RpcValue::Decimal value);
	ChainPackWriter& write_p(RpcValue::DateTime value);
	ChainPackWriter& write_p(const std::string &value);
	//ChainPackWriter& write_p(const RpcValue::Blob &value);
	ChainPackWriter& write_p(const RpcValue::List &values);
	//ChainPackWriter& write_p(const RpcValue::Array &values);
	ChainPackWriter& write_p(const RpcValue::Map &values);
	ChainPackWriter& write_p(const RpcValue::IMap &values);
};

} // namespace chainpack
} // namespace shv
