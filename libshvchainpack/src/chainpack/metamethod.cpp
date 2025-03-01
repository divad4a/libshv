#include <shv/chainpack/metamethod.h>

using namespace std::string_literals;

namespace shv::chainpack {

namespace {
constexpr auto KEY_ACCESSGRANT = "accessGrant";
constexpr auto VOID_TYPE_NAME = "Null";
}

MetaMethod::Signal::Signal(std::string _name, std::optional<std::string> _param_type)
	: name(_name)
	, param_type(_param_type)
{
}

MetaMethod::MetaMethod() = default;

MetaMethod::MetaMethod(std::string name,
	unsigned flags,
	std::optional<std::string> param,
	std::optional<std::string> result,
	AccessLevel access_level,
	const std::vector<Signal>& signal_definitions,
	const std::string& description,
	const std::string& label,
	const RpcValue::Map& extra
)
	: m_name(name)
	, m_flags(flags)
	, m_param(param.value_or(VOID_TYPE_NAME))
	, m_result(result.value_or(VOID_TYPE_NAME))
	, m_accessLevel(access_level)
	, m_label(label)
	, m_description(description)
	, m_extra(extra)
{
	for (const auto& [signal_name, signal_ret] : signal_definitions) {
		m_signals.emplace(signal_name, signal_ret.has_value()? RpcValue(signal_ret.value()): RpcValue(nullptr));
	}
}

MetaMethod::MetaMethod(std::string name,
	Signature signature,
	unsigned int flags,
	const std::string &access_grant,
	const std::string &description,
	const RpcValue::Map &extra)
	: m_name(name)
	, m_flags(flags)
	, m_accessLevel(accessLevelFromAccessString(access_grant).value_or(AccessLevel::Browse))
	, m_description(description)
	, m_extra(extra)
{
	switch (signature) {
	case Signature::VoidVoid: m_result = VOID_TYPE_NAME; m_param = VOID_TYPE_NAME; break;
	case Signature::VoidParam: m_result = VOID_TYPE_NAME; m_param = "RpcValue"; break;
	case Signature::RetVoid: m_result = "RpcValue"; m_param = VOID_TYPE_NAME; break;
	case Signature::RetParam: m_result = "RpcValue"; m_param = "RpcValue"; break;
	}
}

bool MetaMethod::isValid() const
{
	return !name().empty();
}

const std::string& MetaMethod::name() const
{
	return m_name;
}

const std::string &MetaMethod::resultType() const
{
	return m_result;
}

bool MetaMethod::hasResultType() const
{
	return !(m_result.empty() || m_result == VOID_TYPE_NAME);
}

const std::string &MetaMethod::paramType() const
{
	return m_param;
}

bool MetaMethod::hasParamType() const
{
	return !(m_param.empty() || m_param == VOID_TYPE_NAME);
}

const std::string& MetaMethod::label() const
{
	return m_label;
}

MetaMethod& MetaMethod::setLabel(const std::string &label)
{
	m_label = label;
	return *this;
}

const std::string& MetaMethod::description() const
{
	return m_description;
}

unsigned MetaMethod::flags() const
{
	return m_flags;
}

AccessLevel MetaMethod::accessLevel() const
{
	return m_accessLevel;
}

void MetaMethod::setAccessLevel(std::optional<AccessLevel> level)
{
	m_accessLevel = level.value_or(AccessLevel::None);
}

const RpcValue::Map& MetaMethod::extra() const
{
	return m_extra;
}

RpcValue MetaMethod::tag(const std::string &key, const RpcValue &default_value) const
{
	return m_extra.value(key, default_value);
}

MetaMethod &MetaMethod::setTag(const std::string &key, const RpcValue &value)
{
	m_extra.setValue(key, value);
	return *this;
}

enum class Ikey {
	Name = 1,
	Flags = 2,
	ParamType = 3,
	ResultType = 4,
	AccessLevel = 5,
	Signals = 6,
	Extra = 7,
};

RpcValue MetaMethod::toRpcValue() const
{
	RpcValue::IMap ret;
	ret[static_cast<int>(Ikey::Name)] = m_name;
	ret[static_cast<int>(Ikey::ParamType)] = m_param;
	ret[static_cast<int>(Ikey::ResultType)] = m_result;
	ret[static_cast<int>(Ikey::AccessLevel)] = static_cast<RpcValue::Int>(m_accessLevel);
	ret[static_cast<int>(Ikey::Signals)] = m_signals;
	RpcValue::Map extra{
		{KEY_DESCRIPTION, m_description},
		{KEY_LABEL, m_label}
	};
	for (const auto& [k, v] : m_extra) {
		extra.insert_or_assign(k, v);
	}
	ret[static_cast<int>(Ikey::Extra)] = extra;
	return ret;
}
/*
RpcValue MetaMethod::toIMap() const
{
	RpcValue::IMap ret;
	ret[DirKey::Name] = m_name;
	ret[DirKey::Signature] = static_cast<int>(m_signature);
	ret[DirKey::Flags] = m_flags;
	ret[DirKey::Access] = m_accessLevel;
	if(!m_label.empty())
		ret[DirKey::Label] = m_label;
	if(!m_description.empty())
		ret[DirKey::Description] = m_description;
	RpcValue::Map tags = m_tags;
	for(const auto &k : NO_TAGS_KEYS) {
		tags.erase(k);
	}
	if(!tags.empty()) {
		ret[DirKey::Tags] = tags;
	}
	return ret;
}
*/

MetaMethod MetaMethod::fromRpcValue(const RpcValue &rv)
{
	MetaMethod ret;
	if(rv.isString()) {
		ret.m_name = rv.asString();
	}
	else if(rv.isList()) {
		const auto &lst = rv.asList();
		ret.m_name = lst.value(0).asString();
		ret.m_flags = lst.value(2).toUInt();
		ret.setAccessLevel(accessLevelFromAccessString(lst.value(3).asString()).value_or(AccessLevel::None));
		ret.m_description = lst.value(4).asString();
		const auto tags = lst.value(5);
		ret.applyAttributesMap(tags.asMap());
	}
	else if(rv.isMap()) {
		ret.applyAttributesMap(rv.asMap());
	}
	else if(rv.isIMap()) {
		ret.applyAttributesIMap(rv.asIMap());
	}
	return ret;
}

void MetaMethod::applyAttributesMap(const RpcValue::Map &attr_map)
{
	auto map = attr_map;
	if(auto rv = map.take(KEY_NAME); rv.isString())
		m_name = rv.asString();
	if(auto rv = map.take(KEY_FLAGS); rv.isValid())
		m_flags = rv.toInt();
	if(auto rv = map.take(KEY_ACCESS); rv.isString())
		setAccessLevel(accessLevelFromAccessString(rv.asString()));
	if(auto rv = map.take(KEY_ACCESSGRANT); rv.isString())
		setAccessLevel(accessLevelFromAccessString(rv.asString()));
	if(auto rv = map.take(KEY_ACCESS_LEVEL); rv.isInt())
		setAccessLevel(accessLevelFromInt(rv.toInt()));
	if(auto rv = map.take(KEY_LABEL); rv.isString())
		m_label = rv.asString();
	if(auto rv = map.take(KEY_DESCRIPTION); rv.isString())
		m_description = rv.asString();
	if(auto rv = map.take(KEY_TAGS); rv.isMap())
		m_extra = rv.asMap();
}

void MetaMethod::applyAttributesIMap(const RpcValue::IMap &attr_map)
{
	if(auto rv = attr_map.value(static_cast<int>(Ikey::Name)); rv.isString())
		m_name = rv.asString();
	if(auto rv = attr_map.value(static_cast<int>(Ikey::Flags)); rv.isInt())
		m_flags = rv.toInt();
	if(auto rv = attr_map.value(static_cast<int>(Ikey::ParamType)); rv.isString())
		m_param = rv.asString();
	if(auto rv = attr_map.value(static_cast<int>(Ikey::ResultType)); rv.isString())
		m_result = rv.asString();
	{
		auto rv = attr_map.value(static_cast<int>(Ikey::AccessLevel));
		if (rv.isString()) {
			setAccessLevel(accessLevelFromAccessString(rv.asString()));
		}
		if (rv.isInt()) {
			setAccessLevel(accessLevelFromInt(rv.toInt()));
		}
	}
	if(auto rv = attr_map.value(static_cast<int>(Ikey::Signals)); rv.isMap())
		m_signals = rv.asMap();
	if(auto rv = attr_map.value(static_cast<int>(Ikey::Extra)); rv.isMap()) {
		m_extra = rv.asMap();
		m_description = rv.asMap().value(KEY_DESCRIPTION).asString();
		m_label = rv.asMap().value(KEY_LABEL).asString();
	}
}

MetaMethod::Signature MetaMethod::signatureFromString(const std::string &sigstr)
{
	if(sigstr == "VoidParam") return Signature::VoidParam;
	if(sigstr == "RetVoid") return Signature::RetVoid;
	if(sigstr == "RetParam") return Signature::RetParam;
	return Signature::VoidVoid;
}

const char *MetaMethod::signatureToString(MetaMethod::Signature sig)
{
	switch(sig) {
	case Signature::VoidVoid: return "VoidVoid";
	case Signature::VoidParam: return "VoidParam";
	case Signature::RetVoid: return "RetVoid";
	case Signature::RetParam: return "RetParam";
	}
	return "";
}

std::string MetaMethod::flagsToString(unsigned flags)
{
	std::string ret;
	auto add_str = [&ret](const char *str) {
		if(ret.empty()) {
			ret = str;
		}
		else {
			ret += ',';
			ret += str;
		}
		return ret;
	};
	if(flags & Flag::IsGetter)
		add_str("Getter");
	if(flags & Flag::IsSetter)
		add_str("Setter");
	if(flags & Flag::IsSignal)
		add_str("Signal");
	return ret;
}

} // namespace shv
