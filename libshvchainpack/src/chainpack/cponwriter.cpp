#include "cponwriter.h"
#include "cpon.h"
#include "exception.h"

#include "../../c/ccpon.h"

#include <iostream>
#include <cmath>
#include <sstream>

#include <string.h>

namespace shv {
namespace chainpack {

CponWriter::CponWriter(std::ostream &out, const CponWriterOptions &opts)
	: Super(out)
	, m_opts(opts)
{
	m_outCtx.cpon_options.json_output = opts.isJsonFormat();
	m_outCtx.cpon_options.indent = m_opts.indent().empty()? nullptr: m_opts.indent().data();
}

void CponWriter::write(const RpcValue &value)
{
	if(!value.metaData().isEmpty()) {
		write(value.metaData());
	}
	switch (value.type()) {
	case RpcValue::Type::Null: write_p(nullptr); break;
	case RpcValue::Type::UInt: write_p(value.toUInt64()); break;
	case RpcValue::Type::Int: write_p(value.toInt64()); break;
	case RpcValue::Type::Double: write_p(value.toDouble()); break;
	case RpcValue::Type::Bool: write_p(value.toBool()); break;
	//case RpcValue::Type::Blob: write(value.toBlob()); break;
	case RpcValue::Type::String: write_p(value.toString()); break;
	case RpcValue::Type::DateTime: write_p(value.toDateTime()); break;
	case RpcValue::Type::List: write_p(value.toList()); break;
	case RpcValue::Type::Map: write_p(value.toMap()); break;
	case RpcValue::Type::IMap: write_p(value.toIMap(), &value.metaData()); break;
	case RpcValue::Type::Decimal: write_p(value.toDecimal()); break;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			write_p(nullptr);
		}
		break;
	}
}

void CponWriter::write(const RpcValue::MetaData &meta_data)
{
	if(!meta_data.isEmpty()) {
		writeMetaBegin();
		const RpcValue::IMap &cim = meta_data.iValues();
		if(!cim.empty()) {
			for (const auto &kv : cim) {
				if(m_opts.isTranslateIds()) {
					ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
					ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0);
					int nsid = meta_data.metaTypeNameSpaceId();
					int mtid = meta_data.metaTypeId();
					int tag = kv.first;
					const meta::MetaInfo &tag_info = meta::registeredType(nsid, mtid).tagById(tag);
					if(tag_info.isValid())
						ccpcp_pack_copy_bytes(&m_outCtx, tag_info.name, ::strlen(tag_info.name));
					else
						write(tag);
					ccpon_pack_key_val_delim(&m_outCtx);
					RpcValue meta_val = kv.second;
					if(tag == meta::Tag::MetaTypeNameSpaceId) {
						int id = meta_val.toInt();
						const meta::MetaNameSpace &type = meta::registeredNameSpace(nsid);
						const char *n = type.name();
						if(n[0])
							ccpcp_pack_copy_bytes(&m_outCtx, n, ::strlen(n));
						else
							write(id);
					}
					else if(tag == meta::Tag::MetaTypeId) {
						int id = meta_val.toInt();
						const meta::MetaType &type = meta::registeredType(nsid, id);
						const char *n = type.name();
						if(n[0])
							ccpcp_pack_copy_bytes(&m_outCtx, n, ::strlen(n));
						else
							write(id);
					}
					else {
						write(meta_val);
					}
				}
				else {
					writeMapElement(kv.first, kv.second);
				}
			}
		}
		const RpcValue::Map &csm = meta_data.sValues();
		for (const auto &kv : csm) {
			writeMapElement(kv.first, kv.second);
		}
		writeMetaEnd();
	}
}

void CponWriter::writeMetaBegin()
{
	ccpon_pack_meta_begin(&m_outCtx);
	m_containerStates.push_back(ContainerState());
}

void CponWriter::writeMetaEnd()
{
	ccpon_pack_meta_end(&m_outCtx);
	m_containerStates.pop_back();
}

void CponWriter::writeContainerBegin(RpcValue::Type container_type)
{
	switch (container_type) {
	case RpcValue::Type::List:
		ccpon_pack_list_begin(&m_outCtx);
		break;
	case RpcValue::Type::Map:
		ccpon_pack_map_begin(&m_outCtx);
		break;
	case RpcValue::Type::IMap:
		ccpon_pack_imap_begin(&m_outCtx);
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write begin of container type: ") + RpcValue::typeToName(container_type));
	}
	m_containerStates.push_back(ContainerState(container_type));
}

void CponWriter::writeContainerEnd()
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	switch (cs.containerType) {
	case RpcValue::Type::List:
		ccpon_pack_list_end(&m_outCtx);
		break;
	case RpcValue::Type::Map:
		ccpon_pack_map_end(&m_outCtx);
		break;
	case RpcValue::Type::IMap:
		ccpon_pack_imap_end(&m_outCtx);
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write end of container type: ") + RpcValue::typeToName(cs.containerType));
	}
	m_containerStates.pop_back();
}

void CponWriter::writeMapKey(const std::string &key)
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0);
	write(key);
	ccpon_pack_key_val_delim(&m_outCtx);
}

void CponWriter::writeIMapKey(RpcValue::Int key)
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0);
	write(key);
	ccpon_pack_key_val_delim(&m_outCtx);
}

void CponWriter::writeListElement(const RpcValue &val)
{
	ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
	ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0);
	write(val);
}

void CponWriter::writeMapElement(const std::string &key, const RpcValue &val)
{
	writeMapKey(key);
	write(val);
}

void CponWriter::writeMapElement(RpcValue::Int key, const RpcValue &val)
{
	writeIMapKey(key);
	write(val);
}

void CponWriter::writeRawData(const std::string &data)
{
	ccpcp_pack_copy_bytes(&m_outCtx, data.data(), data.size());
}

CponWriter &CponWriter::write_p(std::nullptr_t)
{
	ccpon_pack_null(&m_outCtx);
	return *this;
}

CponWriter &CponWriter::write_p(bool value)
{
	ccpon_pack_boolean(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(int32_t value)
{
	ccpon_pack_int(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(uint32_t value)
{
	ccpon_pack_uint(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(int64_t value)
{
	ccpon_pack_int(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(uint64_t value)
{
	ccpon_pack_uint(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(double value)
{
	ccpon_pack_double(&m_outCtx, value);
	return *this;
}

CponWriter &CponWriter::write_p(RpcValue::Decimal value)
{
	ccpon_pack_decimal(&m_outCtx, value.mantisa(), value.exponent());
	return *this;
}

CponWriter &CponWriter::write_p(RpcValue::DateTime value)
{
	ccpon_pack_date_time(&m_outCtx, value.msecsSinceEpoch(), value.minutesFromUtc());
	return *this;
}

CponWriter &CponWriter::write_p(const std::string &value)
{
	ccpon_pack_string(&m_outCtx, value.data(), value.size());
	return *this;
}

CponWriter &CponWriter::write_p(const RpcValue::Map &values)
{
	writeContainerBegin(RpcValue::Type::Map);
	for (const auto &kv : values) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd();
	return *this;
}

CponWriter &CponWriter::write_p(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data)
{
	writeContainerBegin(RpcValue::Type::IMap);
	for (const auto &kv : values) {
		if(m_opts.isTranslateIds() && meta_data) {
			ContainerState &cs = m_containerStates[m_containerStates.size() - 1];
			ccpon_pack_field_delim(&m_outCtx, cs.elementCount++ == 0);
			int mtid = meta_data->metaTypeId();
			int nsid = meta_data->metaTypeNameSpaceId();
			auto key = kv.first;
			const meta::MetaInfo key_info = meta::registeredType(nsid, mtid).keyById(key);
			if(key_info.isValid()) {
				ccpcp_pack_copy_bytes(&m_outCtx, key_info.name, ::strlen(key_info.name));
			}
			else {
				write(key);
			}
			ccpon_pack_key_val_delim(&m_outCtx);
			write(kv.second);
		}
		else {
			writeMapElement(kv.first, kv.second);
		}
	}
	writeContainerEnd();
	return *this;
}

CponWriter &CponWriter::write_p(const RpcValue::List &values)
{
	writeContainerBegin(RpcValue::Type::List);
	for (size_t ix = 0; ix < values.size(); ix++) {
		const RpcValue &value = values[ix];
		writeListElement(value);
	}
	writeContainerEnd();
	return *this;
}

} // namespace chainpack
} // namespace shv
