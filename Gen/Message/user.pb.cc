// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: user.proto

#include "user.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

namespace user {
class account_infoDefaultTypeInternal {
 public:
  ::google::protobuf::internal::ExplicitlyConstructed<account_info> _instance;
} _account_info_default_instance_;
}  // namespace user
static void InitDefaultsaccount_info_user_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::user::_account_info_default_instance_;
    new (ptr) ::user::account_info();
    ::google::protobuf::internal::OnShutdownDestroyMessage(ptr);
  }
  ::user::account_info::InitAsDefaultInstance();
}

::google::protobuf::internal::SCCInfo<0> scc_info_account_info_user_2eproto =
    {{ATOMIC_VAR_INIT(::google::protobuf::internal::SCCInfoBase::kUninitialized), 0, InitDefaultsaccount_info_user_2eproto}, {}};

void InitDefaults_user_2eproto() {
  ::google::protobuf::internal::InitSCC(&scc_info_account_info_user_2eproto.base);
}

::google::protobuf::Metadata file_level_metadata_user_2eproto[1];
constexpr ::google::protobuf::EnumDescriptor const** file_level_enum_descriptors_user_2eproto = nullptr;
constexpr ::google::protobuf::ServiceDescriptor const** file_level_service_descriptors_user_2eproto = nullptr;

const ::google::protobuf::uint32 TableStruct_user_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::user::account_info, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::user::account_info, account_),
  PROTOBUF_FIELD_OFFSET(::user::account_info, phone_num_),
  PROTOBUF_FIELD_OFFSET(::user::account_info, pass_word_),
  PROTOBUF_FIELD_OFFSET(::user::account_info, user_id_),
  PROTOBUF_FIELD_OFFSET(::user::account_info, login_time_),
  PROTOBUF_FIELD_OFFSET(::user::account_info, register_time_),
  PROTOBUF_FIELD_OFFSET(::user::account_info, last_login_ip_),
  PROTOBUF_FIELD_OFFSET(::user::account_info, is_new_user_),
};
static const ::google::protobuf::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::user::account_info)},
};

static ::google::protobuf::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::google::protobuf::Message*>(&::user::_account_info_default_instance_),
};

::google::protobuf::internal::AssignDescriptorsTable assign_descriptors_table_user_2eproto = {
  {}, AddDescriptors_user_2eproto, "user.proto", schemas,
  file_default_instances, TableStruct_user_2eproto::offsets,
  file_level_metadata_user_2eproto, 1, file_level_enum_descriptors_user_2eproto, file_level_service_descriptors_user_2eproto,
};

const char descriptor_table_protodef_user_2eproto[] =
  "\n\nuser.proto\022\004user\"\255\001\n\014account_info\022\017\n\007a"
  "ccount\030\001 \001(\t\022\021\n\tphone_num\030\002 \001(\003\022\021\n\tpass_"
  "word\030\003 \001(\t\022\017\n\007user_id\030\004 \001(\003\022\022\n\nlogin_tim"
  "e\030\005 \001(\003\022\025\n\rregister_time\030\006 \001(\003\022\025\n\rlast_l"
  "ogin_ip\030\007 \001(\t\022\023\n\013is_new_user\030\010 \001(\010b\006prot"
  "o3"
  ;
::google::protobuf::internal::DescriptorTable descriptor_table_user_2eproto = {
  false, InitDefaults_user_2eproto, 
  descriptor_table_protodef_user_2eproto,
  "user.proto", &assign_descriptors_table_user_2eproto, 202,
};

void AddDescriptors_user_2eproto() {
  static constexpr ::google::protobuf::internal::InitFunc deps[1] =
  {
  };
 ::google::protobuf::internal::AddDescriptors(&descriptor_table_user_2eproto, deps, 0);
}

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_user_2eproto = []() { AddDescriptors_user_2eproto(); return true; }();
namespace user {

// ===================================================================

void account_info::InitAsDefaultInstance() {
}
class account_info::HasBitSetters {
 public:
};

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int account_info::kAccountFieldNumber;
const int account_info::kPhoneNumFieldNumber;
const int account_info::kPassWordFieldNumber;
const int account_info::kUserIdFieldNumber;
const int account_info::kLoginTimeFieldNumber;
const int account_info::kRegisterTimeFieldNumber;
const int account_info::kLastLoginIpFieldNumber;
const int account_info::kIsNewUserFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

account_info::account_info()
  : ::google::protobuf::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:user.account_info)
}
account_info::account_info(const account_info& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(nullptr) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  account_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.account().size() > 0) {
    account_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.account_);
  }
  pass_word_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.pass_word().size() > 0) {
    pass_word_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.pass_word_);
  }
  last_login_ip_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.last_login_ip().size() > 0) {
    last_login_ip_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.last_login_ip_);
  }
  ::memcpy(&phone_num_, &from.phone_num_,
    static_cast<size_t>(reinterpret_cast<char*>(&is_new_user_) -
    reinterpret_cast<char*>(&phone_num_)) + sizeof(is_new_user_));
  // @@protoc_insertion_point(copy_constructor:user.account_info)
}

void account_info::SharedCtor() {
  ::google::protobuf::internal::InitSCC(
      &scc_info_account_info_user_2eproto.base);
  account_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  pass_word_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  last_login_ip_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&phone_num_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&is_new_user_) -
      reinterpret_cast<char*>(&phone_num_)) + sizeof(is_new_user_));
}

account_info::~account_info() {
  // @@protoc_insertion_point(destructor:user.account_info)
  SharedDtor();
}

void account_info::SharedDtor() {
  account_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  pass_word_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  last_login_ip_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}

void account_info::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const account_info& account_info::default_instance() {
  ::google::protobuf::internal::InitSCC(&::scc_info_account_info_user_2eproto.base);
  return *internal_default_instance();
}


void account_info::Clear() {
// @@protoc_insertion_point(message_clear_start:user.account_info)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  account_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  pass_word_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  last_login_ip_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(&phone_num_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&is_new_user_) -
      reinterpret_cast<char*>(&phone_num_)) + sizeof(is_new_user_));
  _internal_metadata_.Clear();
}

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
const char* account_info::_InternalParse(const char* begin, const char* end, void* object,
                  ::google::protobuf::internal::ParseContext* ctx) {
  auto msg = static_cast<account_info*>(object);
  ::google::protobuf::int32 size; (void)size;
  int depth; (void)depth;
  ::google::protobuf::uint32 tag;
  ::google::protobuf::internal::ParseFunc parser_till_end; (void)parser_till_end;
  auto ptr = begin;
  while (ptr < end) {
    ptr = ::google::protobuf::io::Parse32(ptr, &tag);
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    switch (tag >> 3) {
      // string account = 1;
      case 1: {
        if (static_cast<::google::protobuf::uint8>(tag) != 10) goto handle_unusual;
        ptr = ::google::protobuf::io::ReadSize(ptr, &size);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        ctx->extra_parse_data().SetFieldName("user.account_info.account");
        object = msg->mutable_account();
        if (size > end - ptr + ::google::protobuf::internal::ParseContext::kSlopBytes) {
          parser_till_end = ::google::protobuf::internal::GreedyStringParserUTF8;
          goto string_till_end;
        }
        GOOGLE_PROTOBUF_PARSER_ASSERT(::google::protobuf::internal::StringCheckUTF8(ptr, size, ctx));
        ::google::protobuf::internal::InlineGreedyStringParser(object, ptr, size, ctx);
        ptr += size;
        break;
      }
      // int64 phone_num = 2;
      case 2: {
        if (static_cast<::google::protobuf::uint8>(tag) != 16) goto handle_unusual;
        msg->set_phone_num(::google::protobuf::internal::ReadVarint(&ptr));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      // string pass_word = 3;
      case 3: {
        if (static_cast<::google::protobuf::uint8>(tag) != 26) goto handle_unusual;
        ptr = ::google::protobuf::io::ReadSize(ptr, &size);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        ctx->extra_parse_data().SetFieldName("user.account_info.pass_word");
        object = msg->mutable_pass_word();
        if (size > end - ptr + ::google::protobuf::internal::ParseContext::kSlopBytes) {
          parser_till_end = ::google::protobuf::internal::GreedyStringParserUTF8;
          goto string_till_end;
        }
        GOOGLE_PROTOBUF_PARSER_ASSERT(::google::protobuf::internal::StringCheckUTF8(ptr, size, ctx));
        ::google::protobuf::internal::InlineGreedyStringParser(object, ptr, size, ctx);
        ptr += size;
        break;
      }
      // int64 user_id = 4;
      case 4: {
        if (static_cast<::google::protobuf::uint8>(tag) != 32) goto handle_unusual;
        msg->set_user_id(::google::protobuf::internal::ReadVarint(&ptr));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      // int64 login_time = 5;
      case 5: {
        if (static_cast<::google::protobuf::uint8>(tag) != 40) goto handle_unusual;
        msg->set_login_time(::google::protobuf::internal::ReadVarint(&ptr));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      // int64 register_time = 6;
      case 6: {
        if (static_cast<::google::protobuf::uint8>(tag) != 48) goto handle_unusual;
        msg->set_register_time(::google::protobuf::internal::ReadVarint(&ptr));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      // string last_login_ip = 7;
      case 7: {
        if (static_cast<::google::protobuf::uint8>(tag) != 58) goto handle_unusual;
        ptr = ::google::protobuf::io::ReadSize(ptr, &size);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        ctx->extra_parse_data().SetFieldName("user.account_info.last_login_ip");
        object = msg->mutable_last_login_ip();
        if (size > end - ptr + ::google::protobuf::internal::ParseContext::kSlopBytes) {
          parser_till_end = ::google::protobuf::internal::GreedyStringParserUTF8;
          goto string_till_end;
        }
        GOOGLE_PROTOBUF_PARSER_ASSERT(::google::protobuf::internal::StringCheckUTF8(ptr, size, ctx));
        ::google::protobuf::internal::InlineGreedyStringParser(object, ptr, size, ctx);
        ptr += size;
        break;
      }
      // bool is_new_user = 8;
      case 8: {
        if (static_cast<::google::protobuf::uint8>(tag) != 64) goto handle_unusual;
        msg->set_is_new_user(::google::protobuf::internal::ReadVarint(&ptr));
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        break;
      }
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->EndGroup(tag);
          return ptr;
        }
        auto res = UnknownFieldParse(tag, {_InternalParse, msg},
          ptr, end, msg->_internal_metadata_.mutable_unknown_fields(), ctx);
        ptr = res.first;
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr != nullptr);
        if (res.second) return ptr;
      }
    }  // switch
  }  // while
  return ptr;
string_till_end:
  static_cast<::std::string*>(object)->clear();
  static_cast<::std::string*>(object)->reserve(size);
  goto len_delim_till_end;
len_delim_till_end:
  return ctx->StoreAndTailCall(ptr, end, {_InternalParse, msg},
                               {parser_till_end, object}, size);
}
#else  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
bool account_info::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!PROTOBUF_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:user.account_info)
  for (;;) {
    ::std::pair<::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // string account = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (10 & 0xFF)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_account()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->account().data(), static_cast<int>(this->account().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "user.account_info.account"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // int64 phone_num = 2;
      case 2: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (16 & 0xFF)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &phone_num_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // string pass_word = 3;
      case 3: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (26 & 0xFF)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_pass_word()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->pass_word().data(), static_cast<int>(this->pass_word().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "user.account_info.pass_word"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // int64 user_id = 4;
      case 4: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (32 & 0xFF)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &user_id_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // int64 login_time = 5;
      case 5: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (40 & 0xFF)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &login_time_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // int64 register_time = 6;
      case 6: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (48 & 0xFF)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &register_time_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // string last_login_ip = 7;
      case 7: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (58 & 0xFF)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_last_login_ip()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->last_login_ip().data(), static_cast<int>(this->last_login_ip().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "user.account_info.last_login_ip"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // bool is_new_user = 8;
      case 8: {
        if (static_cast< ::google::protobuf::uint8>(tag) == (64 & 0xFF)) {

          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &is_new_user_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, _internal_metadata_.mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:user.account_info)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:user.account_info)
  return false;
#undef DO_
}
#endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER

void account_info::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:user.account_info)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string account = 1;
  if (this->account().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->account().data(), static_cast<int>(this->account().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "user.account_info.account");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->account(), output);
  }

  // int64 phone_num = 2;
  if (this->phone_num() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(2, this->phone_num(), output);
  }

  // string pass_word = 3;
  if (this->pass_word().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->pass_word().data(), static_cast<int>(this->pass_word().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "user.account_info.pass_word");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      3, this->pass_word(), output);
  }

  // int64 user_id = 4;
  if (this->user_id() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(4, this->user_id(), output);
  }

  // int64 login_time = 5;
  if (this->login_time() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(5, this->login_time(), output);
  }

  // int64 register_time = 6;
  if (this->register_time() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(6, this->register_time(), output);
  }

  // string last_login_ip = 7;
  if (this->last_login_ip().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->last_login_ip().data(), static_cast<int>(this->last_login_ip().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "user.account_info.last_login_ip");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      7, this->last_login_ip(), output);
  }

  // bool is_new_user = 8;
  if (this->is_new_user() != 0) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(8, this->is_new_user(), output);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        _internal_metadata_.unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:user.account_info)
}

::google::protobuf::uint8* account_info::InternalSerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:user.account_info)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string account = 1;
  if (this->account().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->account().data(), static_cast<int>(this->account().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "user.account_info.account");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->account(), target);
  }

  // int64 phone_num = 2;
  if (this->phone_num() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(2, this->phone_num(), target);
  }

  // string pass_word = 3;
  if (this->pass_word().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->pass_word().data(), static_cast<int>(this->pass_word().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "user.account_info.pass_word");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        3, this->pass_word(), target);
  }

  // int64 user_id = 4;
  if (this->user_id() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(4, this->user_id(), target);
  }

  // int64 login_time = 5;
  if (this->login_time() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(5, this->login_time(), target);
  }

  // int64 register_time = 6;
  if (this->register_time() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(6, this->register_time(), target);
  }

  // string last_login_ip = 7;
  if (this->last_login_ip().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->last_login_ip().data(), static_cast<int>(this->last_login_ip().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "user.account_info.last_login_ip");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        7, this->last_login_ip(), target);
  }

  // bool is_new_user = 8;
  if (this->is_new_user() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::WriteBoolToArray(8, this->is_new_user(), target);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:user.account_info)
  return target;
}

size_t account_info::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:user.account_info)
  size_t total_size = 0;

  if (_internal_metadata_.have_unknown_fields()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        _internal_metadata_.unknown_fields());
  }
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string account = 1;
  if (this->account().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->account());
  }

  // string pass_word = 3;
  if (this->pass_word().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->pass_word());
  }

  // string last_login_ip = 7;
  if (this->last_login_ip().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->last_login_ip());
  }

  // int64 phone_num = 2;
  if (this->phone_num() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int64Size(
        this->phone_num());
  }

  // int64 user_id = 4;
  if (this->user_id() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int64Size(
        this->user_id());
  }

  // int64 login_time = 5;
  if (this->login_time() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int64Size(
        this->login_time());
  }

  // int64 register_time = 6;
  if (this->register_time() != 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int64Size(
        this->register_time());
  }

  // bool is_new_user = 8;
  if (this->is_new_user() != 0) {
    total_size += 1 + 1;
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void account_info::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:user.account_info)
  GOOGLE_DCHECK_NE(&from, this);
  const account_info* source =
      ::google::protobuf::DynamicCastToGenerated<account_info>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:user.account_info)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:user.account_info)
    MergeFrom(*source);
  }
}

void account_info::MergeFrom(const account_info& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:user.account_info)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.account().size() > 0) {

    account_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.account_);
  }
  if (from.pass_word().size() > 0) {

    pass_word_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.pass_word_);
  }
  if (from.last_login_ip().size() > 0) {

    last_login_ip_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.last_login_ip_);
  }
  if (from.phone_num() != 0) {
    set_phone_num(from.phone_num());
  }
  if (from.user_id() != 0) {
    set_user_id(from.user_id());
  }
  if (from.login_time() != 0) {
    set_login_time(from.login_time());
  }
  if (from.register_time() != 0) {
    set_register_time(from.register_time());
  }
  if (from.is_new_user() != 0) {
    set_is_new_user(from.is_new_user());
  }
}

void account_info::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:user.account_info)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void account_info::CopyFrom(const account_info& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:user.account_info)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool account_info::IsInitialized() const {
  return true;
}

void account_info::Swap(account_info* other) {
  if (other == this) return;
  InternalSwap(other);
}
void account_info::InternalSwap(account_info* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  account_.Swap(&other->account_, &::google::protobuf::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  pass_word_.Swap(&other->pass_word_, &::google::protobuf::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  last_login_ip_.Swap(&other->last_login_ip_, &::google::protobuf::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  swap(phone_num_, other->phone_num_);
  swap(user_id_, other->user_id_);
  swap(login_time_, other->login_time_);
  swap(register_time_, other->register_time_);
  swap(is_new_user_, other->is_new_user_);
}

::google::protobuf::Metadata account_info::GetMetadata() const {
  ::google::protobuf::internal::AssignDescriptors(&::assign_descriptors_table_user_2eproto);
  return ::file_level_metadata_user_2eproto[kIndexInFileMessages];
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace user
namespace google {
namespace protobuf {
template<> PROTOBUF_NOINLINE ::user::account_info* Arena::CreateMaybeMessage< ::user::account_info >(Arena* arena) {
  return Arena::CreateInternal< ::user::account_info >(arena);
}
}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
