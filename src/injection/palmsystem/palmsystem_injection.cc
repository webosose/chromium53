// Copyright (c) 2014 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/browser_control_messages.h"
#include "content/public/renderer/injection/injection_base.h"
#include "injection/palmsystem/palmsystem_injection.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "luna_service_mgr.h"
#include "pmtracer.h"
#include <string>
#include <time.h>

namespace extensions_v8 {

const char kPalmSystemInjectionName[] = "v8/palmsystem";

const char kPalmSystemInjectionAPI[] =
  "var palmGetResource;"
  "if (typeof(palmGetResource) == 'undefined') {"
  "  palmGetResource = {};"
  "};"

  "palmGetResource = function(p1, p2) {"
  "  return PalmSystem.getResource(p1, p2);"
  "};"

  "var webos;"
  "if (typeof(webos) == 'undefined') {"
  "  webos = {};"
  "};"

  "webos = {"
  "  get timezone() {"
  "    return PalmSystem.timeZone;"
  "  },"
  "};"

  "var PalmServiceBridge;"
  "if (typeof(PalmServiceBridge) == 'undefined') {"
  "  PalmServiceBridge = {};"
  "};"

  "function PalmServiceBridge() {"
  "  native function PalmServiceBridgeConstruct();"
  "  return PalmServiceBridgeConstruct();"
  "};"

  "var PalmSystem;"
  "if (typeof(PalmSystem) == 'undefined') {"
  "  PalmSystem = {};"
  "};"

  "native function PalmSystemConstruct();"
  "PalmSystem = PalmSystemConstruct();"
  "PalmSystem.cursor = PalmSystem.cursorConstruct();"
  "PalmSystem.window = PalmSystem.windowConstruct();"

  "PalmSystem.window.setFocus = function(arg) {"
  "   var value = 'false';"
  "   if(arg == true) value = 'true';"
  "   PalmSystem.window.setProperty('needFocus', value);"
  "};"

  "PalmSystem._onCloseWithNotify_ = function(arg) {"
  "  if (typeof(PalmSystem._onCloseCallback_) != 'undefined') {"
  "    if (typeof(PalmSystem._onCloseCallback_) == 'function') {"
  "      PalmSystem._setAppInClosing_();"
  "      PalmSystem._onCloseCallback_(arg);"
  "      if (PalmSystem._didRunOnCloseCallback_() == true) {"
  "        PalmSystem.onCloseNotify(\"didRunOnCloseCallback\");"
  "      }"
  "    };"
  "  } else {"
  "    console.log('Callback is undefined');"
  "  };"
  "};"

  "Object.defineProperty(PalmSystem, \"onclose\", {"
  "  set: function(p1) {"
  "    if (typeof(p1) == 'function') {"
  "      PalmSystem._onCloseCallback_ = p1;"
  "      PalmSystem.onCloseNotify(\"didSetOnCloseCallback\");"
  "    } else if(typeof(p1) === 'undefined' || p1 === undefined) {"
  "      PalmSystem._onCloseCallback_ = p1;"
  "      PalmSystem.onCloseNotify(\"didClearOnCloseCallback\");"
  "    } else {"
  "      console.log('Parameter is not function');"
  "    };"
  "  },"
  "  get: function() {"
  "    return PalmSystem._onCloseCallback_;"
  "  }"
  "});"

  "PalmSystem.close = function(p1) {"
  "  if(p1 && p1 == 'EXIT_TO_LASTAPP') {"
  "    PalmSystem.window.setProperty('_WEBOS_LAUNCH_PREV_APP_AFTER_CLOSING', 'true');"
  "  }"
  "  if (self !== top)"
  "   top.window.close();"
  "  else"
  "   window.close();"
  "};";

const char kPalmSystemInjectionPendingAPI[] =
  "Object.defineProperty(this, \"devicePixelRatio\", {"
  "  get: function() {"
  "    return PalmSystem.devicePixelRatio();"
  "  }"
  "});";

class InjectionLunaServiceBridge : public LunaServiceManagerListener {
 public:
  static const char kOnServiceCallbackMethodName[];
  static const char kCallMethodName[];
  static const char kCancelMethodName[];

  // To handle luna call in PalmSystem.onclose callback
  static std::set<InjectionLunaServiceBridge*> waiting_responses_;
  static bool is_in_closing_;

  static void CallMethod(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CancelMethod(const v8::FunctionCallbackInfo<v8::Value>& args);

  InjectionLunaServiceBridge() { }
  ~InjectionLunaServiceBridge();

  void Call(const char* uri, const char* payload);
  void Cancel();
  virtual void ServiceResponse(const char* body);

 private:
  friend class PalmSystemInjection;
  void SetupIdentifier();
  void CloseNotify();

  static void NearObjectDeath(
    const v8::WeakCallbackInfo<InjectionLunaServiceBridge>& data);

  v8::Persistent<v8::Object> object_;
  bool canceled_ = false;
  std::string identifier_;
  std::shared_ptr<LunaServiceManager> lsm_;
};

const char InjectionLunaServiceBridge::kOnServiceCallbackMethodName[] =
    "onservicecallback";
const char InjectionLunaServiceBridge::kCallMethodName[] = "call";
const char InjectionLunaServiceBridge::kCancelMethodName[] = "cancel";

// To handle luna call in PalmSystem.onclose callback
bool InjectionLunaServiceBridge::is_in_closing_ = false;
std::set<InjectionLunaServiceBridge*>
    InjectionLunaServiceBridge::waiting_responses_;

// static
void InjectionLunaServiceBridge::CallMethod(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Local<v8::Object> holder = args.Holder();
  v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(
      holder->GetInternalField(0));

  InjectionLunaServiceBridge* self =
      static_cast<InjectionLunaServiceBridge*>(wrap->Value());

  if (!self)
    return;

  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString())
    self->Call("", "");
  else if (args.Length() >= 2)
    self->Call(*v8::String::Utf8Value(args[0]),
               *v8::String::Utf8Value(args[1]));
}

// static
void InjectionLunaServiceBridge::CancelMethod(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Local<v8::Object> holder = args.Holder();
  v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(
      holder->GetInternalField(0));
  InjectionLunaServiceBridge* self =
      static_cast<InjectionLunaServiceBridge*>(wrap->Value());
  if (self)
    self->Cancel();
}

void InjectionLunaServiceBridge::SetupIdentifier() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, "PalmSystem.getIdentifier()");
  v8::Local<v8::Script> script = v8::Script::Compile(source);
  v8::Local<v8::Value> result = script->Run();
  if (!result.IsEmpty() && result->IsString())
    identifier_ = *v8::String::Utf8Value(result);
}

void InjectionLunaServiceBridge::NearObjectDeath(
  const v8::WeakCallbackInfo<InjectionLunaServiceBridge>& data) {
  InjectionLunaServiceBridge* self = data.GetParameter();
  if (self)
    delete self;
}

InjectionLunaServiceBridge::~InjectionLunaServiceBridge() {
  Cancel();

  if (!object_.IsEmpty())
    object_.Reset();
}

void InjectionLunaServiceBridge::Cancel() {
  if (!lsm_ || canceled_)
    return;

  canceled_ = true;
  if (GetListenerToken())
    lsm_->Cancel(this);

  if (InjectionLunaServiceBridge::is_in_closing_)
    waiting_responses_.erase(this);
}

void InjectionLunaServiceBridge::Call(const char* uri, const char* payload) {
  if (identifier_.empty())
    SetupIdentifier();
  if (identifier_.empty())
    return;

  if (!lsm_)
    lsm_ = LunaServiceManager::GetManager(identifier_);

  if (lsm_) {
    if (InjectionLunaServiceBridge::is_in_closing_) {
      std::pair<std::set<InjectionLunaServiceBridge*>::iterator, bool> ret =
          waiting_responses_.insert(this);
      if (!lsm_->Call(uri, payload, this))
        waiting_responses_.erase(ret.first);
    } else
      lsm_->Call(uri, payload, this);
  }
}

void InjectionLunaServiceBridge::ServiceResponse(const char* body) {
  if (object_.IsEmpty() || object_.IsNearDeath())
    return;

  bool shouldCallCloseNotify = false;
  if (InjectionLunaServiceBridge::is_in_closing_) {
    waiting_responses_.erase(this);
    if (waiting_responses_.empty())
      shouldCallCloseNotify = true;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> recv =
      v8::Local<v8::Object>::New(isolate, object_);

  v8::Local<v8::String> callback_key(v8::String::NewFromUtf8(
      isolate, InjectionLunaServiceBridge::kOnServiceCallbackMethodName));

  if (!recv->Has(callback_key))
    return;

  v8::Local<v8::Value> func_value = recv->Get(callback_key);
  if (!func_value->IsFunction())
    return;

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_value);
  v8::Context::Scope context_scope(recv->CreationContext());

  const int argc = 1;
  v8::Local<v8::Value> argv[] = { v8::String::NewFromUtf8(isolate, body) };
  func->Call(recv, argc, argv);

  if (shouldCallCloseNotify) {
    // This function should be executed after context_scope(context)
    // Unless there will be libv8.so crash
    CloseNotify();
  }
}

void InjectionLunaServiceBridge::CloseNotify() {
  if (!InjectionLunaServiceBridge::is_in_closing_ ||
      !InjectionLunaServiceBridge::waiting_responses_.empty())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate,
      "PalmSystem.onCloseNotify(\"didRunOnCloseCallback\")");
  v8::Local<v8::Script> script = v8::Script::Compile(source);
  v8::Local<v8::Value> result = script->Run();
}

class PalmSystem : public InjectionDataManager {
 public:
  PalmSystem(const std::string& json);
  ~PalmSystem();
  bool GetInitialisedStatus() const { return initialised_; }
  void SetInitialisedStatus(bool status) { initialised_ = status; }
  void DoInitialize(const std::string& json);

 private:
  friend class PalmSystemInjection;

  static void NearObjectDeath(
    const v8::WeakCallbackInfo<PalmSystem>& data);

  v8::Local<v8::Value> on_close_callback_;

  v8::Persistent<v8::Object> object_;
  static const std::vector<std::string> cached_data_keys_;
  bool initialised_;
};

const std::vector<std::string> PalmSystem::cached_data_keys_ = {
  "activityId",
  "country",
  "currentCountryGroup",
  "deviceInfo",
  "folderPath",
  "identifier",
  "isMinimal",
  "launchParams",
  "locale",
  "localeRegion",
  "phoneRegion",
  "screenOrientation",
  "timeFormat",
  "timeZone",
  "tvSystemName",
  "devicePixelRatio",
  "trustLevel"
};

PalmSystem::PalmSystem(const std::string& json)
    : initialised_(false) {
  DoInitialize(json);
}

PalmSystem::~PalmSystem() {
  if (!object_.IsEmpty())
    object_.Reset();
}

void PalmSystem::DoInitialize(const std::string& json) {
  if(!initialised_ && json != "") {
    Initialize(json, cached_data_keys_);
    initialised_ = true;
  }
}

void PalmSystem::NearObjectDeath(
  const v8::WeakCallbackInfo<PalmSystem>& data) {
  PalmSystem* self = data.GetParameter();
  if (self)
    delete self;
}

class PalmSystemInjection : public v8::Extension, public InjectionBase {
 public:
  PalmSystemInjection();

  virtual v8::Handle<v8::FunctionTemplate> GetNativeFunctionTemplate(
    v8::Isolate* isolate,
    v8::Handle<v8::String> name) override;
  void InstallPendingFeaturesForExtensions() override;

  static void Activate(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Deactivate(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetWindowOrientation(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetWindowOrientation(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void OnCloseNotify(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetLaunchParams(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetLaunchParams(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetCountry(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetLocale(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetLocaleRegion(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetTimeFormat(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetTimeZone(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetIsMinimal(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetIdentifier(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetScreenOrientation(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetDeviceInfo(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetIsActivated(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetIsKeyboardVisible(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetActivityId(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetPhoneRegion(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void NativePmLogInfoWithClock(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void NativePmLogString(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void NativePmTrace(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void NativePmTraceItem(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void NativePmTraceBefore(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void NativePmTraceAfter(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void AddBannerMessage(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void RemoveBannerMessage(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ClearBannerMessages(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SimulateMouseClick(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void UseSimulatedMouseClicks(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Paste(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CopiedToClipboard(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void PastedFromClipboard(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void MarkFirstUseDone(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void EnableFullScreenMode(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void StagePreparing(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void StageReady(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ContainerReady(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void EditorFocused(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void KeepAlive(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetResource(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DevicePixelRatio(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ApplyLaunchFeedback(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void AddNewContentIndicator(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void RemoveNewContentIndicator(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void KeyboardShow(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void KeyboardHide(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetManualKeyboardEnabled(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void PlatformBack(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetInputRegion(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetKeyMask(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetWindowProperty(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetCursor(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetLoadErrorPolicy(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Hide(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void FocusOwner(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void FocusLayer(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetCursorVisibility(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetCursorState(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ServiceCall(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetAppInClosing(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DidRunOnCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

  static v8::Local<v8::ObjectTemplate> MakeRequestTemplate(v8::Isolate* isolate);
  static void PalmServiceBridgeConstruct(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void PalmSystemConstruct(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetPalmSystemMethodsAndProperties(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate>& object_template);

  static void CursorConstruct(
  const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetCursorMethodsAndProperties(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate>& object_template);

  static void WindowConstruct(
  const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetWindowMethodsAndProperties(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate>& object_template);


  static void ReloadInjectionData(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static void UpdateInjectionData(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static PalmSystem* GetPalmSystemPointer(
    const v8::FunctionCallbackInfo<v8::Value>& args);
  static std::string GetInjectionData(
    const std::string& name,
    const v8::FunctionCallbackInfo<v8::Value>& args);

  static v8::Persistent<v8::ObjectTemplate> request_template_;
};

v8::Persistent<v8::ObjectTemplate> PalmSystemInjection::request_template_;

PalmSystemInjection::PalmSystemInjection()
  : v8::Extension(
      kPalmSystemInjectionName,
      kPalmSystemInjectionAPI) { }

v8::Handle<v8::FunctionTemplate> PalmSystemInjection::GetNativeFunctionTemplate(
    v8::Isolate* isolate,
    v8::Handle<v8::String> name) {

  if (name->Equals(v8::String::NewFromUtf8(isolate, "PalmServiceBridgeConstruct"))) {
    return v8::FunctionTemplate::New(isolate, PalmServiceBridgeConstruct);
  } else if (name->Equals(v8::String::NewFromUtf8(isolate, "PalmSystemConstruct"))) {
    return v8::FunctionTemplate::New(isolate, PalmSystemConstruct);
  }
  return v8::FunctionTemplate::New(isolate, NotImplemented);
}

void PalmSystemInjection::InstallPendingFeaturesForExtensions() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::String> source = v8::String::NewFromUtf8(
      isolate, kPalmSystemInjectionPendingAPI);
  v8::Script::CompileExtension(isolate, source, this);
}

void PalmSystemInjection::Activate(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("activate");
}

void PalmSystemInjection::Deactivate(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("deactivate");
}

void PalmSystemInjection::GetWindowOrientation(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("windowOrientation", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::SetWindowOrientation(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length() || !args[0]->IsString())
    return;

  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  SendCommand("setWindowOrientation", arguments);
}

void PalmSystemInjection::OnCloseNotify(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length() || !args[0]->IsString())
    return;

  RAW_PMLOG_INFO("[PalmSystem]",
                 "PalmSystem.OnCloseNotify(%s)",
                 (*v8::String::Utf8Value(args[0])));
  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  SendCommand("onCloseNotify", arguments);
}

void PalmSystemInjection::GetLaunchParams(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("launchParams", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::SetLaunchParams(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length() || !args[0]->IsString())
    return;

   PalmSystem* palmSystem = GetPalmSystemPointer(args);
   std::string launchParams = *v8::String::Utf8Value(args[0]);
  if (palmSystem)
    palmSystem->UpdateInjectionData("launchParams", launchParams);

  std::vector<std::string> arguments;
  arguments.push_back(launchParams);
  SendCommand("launchParams", arguments);
}

void PalmSystemInjection::GetCountry(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("country", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetLocale(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("locale", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetLocaleRegion(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("localeRegion", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetTimeFormat(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("timeFormat", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetTimeZone(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();

  time_t localTime = time(0);
  tm localTM;
  localTM = *localtime(&localTime);

  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, localTM.tm_zone? localTM.tm_zone: ""));
}

void PalmSystemInjection::GetIsMinimal(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string result = GetInjectionData("isMinimal", args);
  if (result == "true")
      args.GetReturnValue().Set(true);
  else
      args.GetReturnValue().Set(false);
}

void PalmSystemInjection::GetIdentifier(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("identifier", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetScreenOrientation(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("screenOrientation", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetDeviceInfo(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("deviceInfo", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetIsActivated(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string result = GetInjectionData("isActivated", args);
  if (result == "true")
      args.GetReturnValue().Set(true);
  else
      args.GetReturnValue().Set(false);
}

void PalmSystemInjection::GetIsKeyboardVisible(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string result = CallFunction("isKeyboardVisible");
  args.GetReturnValue().Set(result == "true");
}

void PalmSystemInjection::GetActivityId(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("activityId", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::GetPhoneRegion(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("phoneRegion", args);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::NativePmLogInfoWithClock(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 3 || !args[0]->IsString() || !args[1]->IsString()
        || !args[2]->IsString())
        return;

    std::vector<std::string> arguments;
    arguments.push_back(*v8::String::Utf8Value(args[0]));
    arguments.push_back(*v8::String::Utf8Value(args[1]));
    arguments.push_back(*v8::String::Utf8Value(args[2]));
    SendCommand("PmLogInfoWithClock", arguments);
}

void PalmSystemInjection::NativePmLogString(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 4 || !args[0]->IsNumber() || !args[1]->IsString()
    || !args[2]->IsString() || !args[3]->IsString())
    return;

  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  arguments.push_back(*v8::String::Utf8Value(args[1]));
  arguments.push_back(*v8::String::Utf8Value(args[2]));
  arguments.push_back(*v8::String::Utf8Value(args[3]));
  SendCommand("PmLogString", arguments);
}

void PalmSystemInjection::NativePmTrace(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
#if ENABLE_LTTNG
  if (!args.Length() || !args[0]->IsString())
    return;
  v8::String::Utf8Value arg0(args[0]);
  PMTRACE(*arg0);
#endif
}

void PalmSystemInjection::NativePmTraceItem(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
#if ENABLE_LTTNG
  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString())
    return;
  v8::String::Utf8Value arg0(args[0]);
  v8::String::Utf8Value arg1(args[1]);
  PMTRACE_ITEM(*arg0, *arg1);
#endif
}

void PalmSystemInjection::NativePmTraceBefore(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
#if ENABLE_LTTNG
  if (!args.Length() || !args[0]->IsString())
    return;
  v8::String::Utf8Value arg0(args[0]);
  PMTRACE_BEFORE(*arg0);
#endif
}

void PalmSystemInjection::NativePmTraceAfter(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
#if ENABLE_LTTNG
  if (!args.Length() || !args[0]->IsString())
    return;
  v8::String::Utf8Value arg0(args[0]);
  PMTRACE_AFTER(*arg0);
#endif
}

void PalmSystemInjection::AddBannerMessage(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 5 || !args[0]->IsString() || !args[1]->IsString()
    || !args[2]->IsString() || !args[3]->IsString() || !args[4]->IsString())
    return;
  v8::Isolate* isolate = args.GetIsolate();
  std::vector<std::string> arguments;
  for (size_t i = 0; i < 5; i++)
    arguments.push_back(*v8::String::Utf8Value(args[i]));
  std::string result = CallFunction("addBannerMessage", arguments);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::RemoveBannerMessage(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length() || !args[0]->IsString())
    return;

  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  SendCommand("removeBannerMessage", arguments);
}

void PalmSystemInjection::ClearBannerMessages(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("clearBannerMessages");
}

void PalmSystemInjection::SimulateMouseClick(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 3 || !args[0]->IsNumber() || !args[1]->IsNumber()
    || !args[2]->IsBoolean())
    return;
  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  arguments.push_back(*v8::String::Utf8Value(args[1]));
  if (args[2]->IsTrue())
    arguments.push_back("true");
  else
    arguments.push_back("false");
  SendCommand("simulateMouseClick", arguments);
}

void PalmSystemInjection::UseSimulatedMouseClicks(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::vector<std::string> arguments;
  if (args[0]->IsTrue())
    arguments.push_back("true");
  else
    arguments.push_back("false");
  SendCommand("useSimulatedMouseClicks", arguments);
}

void PalmSystemInjection::Paste(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("paste");
}

void PalmSystemInjection::CopiedToClipboard(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  //NOTIMPLEMENTED();
}

void PalmSystemInjection::PastedFromClipboard(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  //NOTIMPLEMENTED();
}

void PalmSystemInjection::MarkFirstUseDone(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  //NOTIMPLEMENTED();
}

void PalmSystemInjection::EnableFullScreenMode(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length() || !args[0]->IsBoolean())
    return;

  std::vector<std::string> arguments;
  if (args[0]->IsTrue())
    arguments.push_back("true");
  else
    arguments.push_back("false");
  SendCommand("enableFullScreenMode", arguments);
}

void PalmSystemInjection::StagePreparing(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("stagePreparing");
}

void PalmSystemInjection::StageReady(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("stageReady");
}

void PalmSystemInjection::ContainerReady(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("containerReady");
}

void PalmSystemInjection::EditorFocused(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  //NOTIMPLEMENTED();
}

void PalmSystemInjection::KeepAlive(
    const v8::FunctionCallbackInfo<v8::Value>& args) {

  std::vector<std::string> arguments;
  if (args[0]->IsTrue())
    arguments.push_back("true");
  else
    arguments.push_back("false");

  // Set websetting first because it is same renderer
  setKeepAliveWebApp(args[0]->IsTrue());

  SendCommand("keepAlive", arguments);
}

void PalmSystemInjection::GetResource(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() && args[0]->IsString()) {
    std::vector<std::string> arguments;
    arguments.push_back(*v8::String::Utf8Value(args[0]));
    const std::string result = CallFunction("getResource", arguments);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
    return;
  }
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, ""));
}

void PalmSystemInjection::ApplyLaunchFeedback(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  //NOTIMPLEMENTED();
}

void PalmSystemInjection::AddNewContentIndicator(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = CallFunction("addNewContentIndicator");
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, result.c_str()));
}

void PalmSystemInjection::RemoveNewContentIndicator(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length())
    return;

  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  SendCommand("removeNewContentIndicator", arguments);
}

void PalmSystemInjection::KeyboardShow(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length() || !args[0]->IsNumber())
    return;

  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  SendCommand("keyboardShow", arguments);
}

void PalmSystemInjection::KeyboardHide(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("keyboardHide");
}

void PalmSystemInjection::SetManualKeyboardEnabled(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length() || !args[0]->IsBoolean())
    return;

  std::vector<std::string> arguments;
  if (args[0]->IsTrue())
    arguments.push_back("true");
  else
    arguments.push_back("false");
  SendCommand("setManualKeyboardEnabled", arguments);
}

void PalmSystemInjection::PlatformBack(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("platformBack");
}

void PalmSystemInjection::SetInputRegion(
    const v8::FunctionCallbackInfo<v8::Value>& args) {

  if (!args.Length())
    return;

  v8::Isolate* isolate = args.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  std::vector<std::string> arguments;
  v8::Local<v8::Value> json_string[] = { v8::Local<v8::Value>::Cast(args[0]) };

  v8::Local<v8::Object> json_object = context->Global()->Get(
      v8::String::NewFromUtf8(isolate, "JSON"))->ToObject();

  v8::Local<v8::Function> stringify_func = v8::Local<v8::Function>::Cast(
      json_object->Get(v8::String::NewFromUtf8(isolate, "stringify")));

  arguments.push_back(*v8::String::Utf8Value(
      stringify_func->Call(json_object, 1, json_string)));

  SendCommand("setInputRegion", arguments);
}

void PalmSystemInjection::SetKeyMask(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.Length())
    return;

  std::string output_js;
  std::vector<std::string> arguments;
  base::ListValue* root  = new base::ListValue();
  v8::Local<v8::Array> array  = v8::Local<v8::Array>::Cast(args[0]);

  for (int i=0; i<array->Length(); i++) {
    v8::Local<v8::Value> item = array->Get(i);
    root->AppendString(*v8::String::Utf8Value(item->ToString()));
  }
  base::JSONWriter::Write(*root, &output_js);

  arguments.push_back(output_js);
  SendCommand("setKeyMask", arguments);
}

void PalmSystemInjection::SetWindowProperty(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString())
    return;

  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  arguments.push_back(*v8::String::Utf8Value(args[1]));
  SendCommand("setWindowProperty", arguments);
}

void PalmSystemInjection::SetCursor(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  // setCursor("defualt")
  // setCursor("blank")
  // setCursor("filePath")
  // setCursor("filePath", hotspot_x, hotspot_y)
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() < 1 || !args[0]->IsString()) {
    args.GetReturnValue().Set(false);
    return;
  }


  std::string x = "0";
  std::string y = "0";
  std::string cursorArg = *v8::String::Utf8Value(args[0]);

  if(args.Length() >= 2 && args[1]->IsNumber())
    x = *v8::String::Utf8Value(args[1]);
  if(args.Length() >= 3 && args[2]->IsNumber())
    y = *v8::String::Utf8Value(args[2]);

  // for a security issue, only app installed file path is allowed
  // PalmSystem.setCursor("/usr/palm/application/myapp/cursor.png"); // set custom cursor
  // PalmSystem.setCursor("file:///usr/palm/application/myapp/cursor.png"); // set custom cursor
  // PalmSystem.setCursor("default"); // restore to default system setting
  // PalmSystem.setCursor(""); // restore to default system setting
  // PalmSystem.setCursor("image/cursor.png"); // set custom cursor
  // PalmSystem.setCursor("image/cursor.png", 9, 10); // set custom cursor with hot spot (pointing tip)
  if(cursorArg.empty())
    cursorArg = "default";
  else if(cursorArg != "default" && cursorArg != "blank") {
    // custom cursor : need to check file path
    cursorArg = checkFileValidation(cursorArg, GetInjectionData("folderPath", args));
    if(cursorArg.empty()) {
      args.GetReturnValue().Set(false);
      return;
    }
  }

  std::vector<std::string> arguments;
  arguments.push_back(cursorArg);
  arguments.push_back(x);
  arguments.push_back(y);
  SendCommand("setCursor", arguments);

  args.GetReturnValue().Set(true);
}

void PalmSystemInjection::SetLoadErrorPolicy(
    const v8::FunctionCallbackInfo<v8::Value>& args) {

  if (args.Length() < 1 || !args[0]->IsString())
    return;
  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(args[0]));
  SendCommand("setLoadErrorPolicy", arguments);
}

void PalmSystemInjection::Hide(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("hide");
}

void PalmSystemInjection::FocusOwner(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("focusOwner");
}

void PalmSystemInjection::FocusLayer(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  SendCommand("focusLayer");
}

void PalmSystemInjection::DevicePixelRatio(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = GetInjectionData("devicePixelRatio", args);
  args.GetReturnValue().Set(std::stod(result));
}

void PalmSystemInjection::GetCursorVisibility(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = CallFunction("cursorVisibility");
  args.GetReturnValue().Set((result == "true") ? true : false);
}

void PalmSystemInjection::GetCursorState(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  // if there are any other states of cursor
  // it should be include as a attribute of the return value
  v8::Isolate* isolate = args.GetIsolate();
  std::string result = CallFunction("cursorVisibility");
  std::string state = "{ \"visibility\" : " + result + " }";
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, state.c_str()));
}

void PalmSystemInjection::ServiceCall(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  // PalmSystem.serviceCall("url", "payload");
  // available for only trustLevel : "trusted"
  // available in app closing
  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
    args.GetReturnValue().Set(false);
    return;
  }

  // check trustLevel : only trusted webapp can call this function
  std::string trustLevel = GetInjectionData("trustLevel", args);
  if (*v8::String::Utf8Value(args[0]), GetInjectionData("trustLevel", args) != "trusted") {
    args.GetReturnValue().Set(false);
    return;
  }

  // This member is static class variable
  // But this value is separated between apps
  if (!InjectionLunaServiceBridge::is_in_closing_) {
    args.GetReturnValue().Set(false);
    return;
  }

  std::string url = *v8::String::Utf8Value(args[0]);
  std::string payload = *v8::String::Utf8Value(args[1]);
  if (url.empty() || payload.empty()) {
    args.GetReturnValue().Set(false);
    return;
  }

  std::vector<std::string> arguments;
  arguments.push_back(url);
  arguments.push_back(payload);
  SendCommand("serviceCall", arguments);
  args.GetReturnValue().Set(true);
}

void PalmSystemInjection::SetAppInClosing(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  // Even if app A set this class static variable to true
  // This does not affect to other apps
  InjectionLunaServiceBridge::is_in_closing_ = true;
}

void PalmSystemInjection::DidRunOnCloseCallback(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(
      InjectionLunaServiceBridge::waiting_responses_.empty());
}

v8::Local<v8::ObjectTemplate> PalmSystemInjection::MakeRequestTemplate(v8::Isolate* isolate) {
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::ObjectTemplate> instance_template = v8::ObjectTemplate::New(isolate);

  instance_template->SetInternalFieldCount(1);

  instance_template->Set(
      v8::String::NewFromUtf8(
          isolate, InjectionLunaServiceBridge::kOnServiceCallbackMethodName),
      v8::FunctionTemplate::New(isolate));

  instance_template->Set(
      v8::String::NewFromUtf8(
          isolate, InjectionLunaServiceBridge::kCallMethodName),
      v8::FunctionTemplate::New(
          isolate, InjectionLunaServiceBridge::CallMethod));

  instance_template->Set(
      v8::String::NewFromUtf8(
          isolate, InjectionLunaServiceBridge::kCancelMethodName),
      v8::FunctionTemplate::New(
          isolate, InjectionLunaServiceBridge::CancelMethod));

  return handle_scope.Escape(instance_template);
}

void PalmSystemInjection::PalmServiceBridgeConstruct(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if(request_template_.IsEmpty()) {
    v8::Handle<v8::ObjectTemplate> raw_template = MakeRequestTemplate(isolate);
    request_template_.Reset(isolate, raw_template);
  }
  v8::Handle<v8::ObjectTemplate> instance_template = v8::Local<v8::ObjectTemplate>::New(isolate, request_template_);

  v8::Local<v8::Object> instance = instance_template->NewInstance();

  InjectionLunaServiceBridge* p = new InjectionLunaServiceBridge();
  p->object_.Reset(isolate, instance);
  p->object_.SetWeak(p, InjectionLunaServiceBridge::NearObjectDeath, v8::WeakCallbackType::kParameter);

  instance->SetInternalField(0, v8::External::New(isolate, p));
  args.GetReturnValue().Set(instance);
}

void PalmSystemInjection::SetPalmSystemMethodsAndProperties(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate>& object_template) {

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "launchParams"),
      v8::FunctionTemplate::New(
          isolate, GetLaunchParams),
      v8::FunctionTemplate::New(
          isolate, SetLaunchParams));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "windowOrientation"),
      v8::FunctionTemplate::New(
          isolate, GetWindowOrientation),
      v8::FunctionTemplate::New(
          isolate, SetWindowOrientation));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "onCloseNotify"),
      v8::FunctionTemplate::New(
          isolate, OnCloseNotify));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "_setAppInClosing_"),
      v8::FunctionTemplate::New(
          isolate, SetAppInClosing));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "_didRunOnCloseCallback_"),
      v8::FunctionTemplate::New(
          isolate, DidRunOnCloseCallback));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setWindowOrientation"),
      v8::FunctionTemplate::New(
          isolate, SetWindowOrientation));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "country"),
      v8::FunctionTemplate::New(
          isolate, GetCountry));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "locale"),
      v8::FunctionTemplate::New(
          isolate, GetLocale));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "localeRegion"),
      v8::FunctionTemplate::New(
          isolate, GetLocaleRegion));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "timeFormat"),
      v8::FunctionTemplate::New(
          isolate, GetTimeFormat));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "timeZone"),
      v8::FunctionTemplate::New(
          isolate, GetTimeZone));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "isMinimal"),
      v8::FunctionTemplate::New(
          isolate, GetIsMinimal));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "identifier"),
      v8::FunctionTemplate::New(
          isolate, GetIdentifier));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "screenOrientation"),
      v8::FunctionTemplate::New(
          isolate, GetScreenOrientation));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "deviceInfo"),
      v8::FunctionTemplate::New(
          isolate, GetDeviceInfo));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "isActivated"),
      v8::FunctionTemplate::New(
          isolate, GetIsActivated));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "isKeyboardVisible"),
      v8::FunctionTemplate::New(
          isolate, GetIsKeyboardVisible));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "activityId"),
      v8::FunctionTemplate::New(
          isolate, GetActivityId));

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "phoneRegion"),
      v8::FunctionTemplate::New(
          isolate, GetPhoneRegion));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "activate"),
      v8::FunctionTemplate::New(
          isolate, Activate));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "deactivate"),
      v8::FunctionTemplate::New(
          isolate, Deactivate));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "PmLogInfoWithClock"),
      v8::FunctionTemplate::New(
          isolate, NativePmLogInfoWithClock));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "PmLogString"),
      v8::FunctionTemplate::New(
          isolate, NativePmLogString));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "PmTrace"),
      v8::FunctionTemplate::New(
          isolate, NativePmTrace));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "PmTraceItem"),
      v8::FunctionTemplate::New(
          isolate, NativePmTraceItem));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "PmTraceBefore"),
      v8::FunctionTemplate::New(
          isolate, NativePmTraceBefore));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "PmTraceAfter"),
      v8::FunctionTemplate::New(
          isolate, NativePmTraceAfter));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "getIdentifier"),
      v8::FunctionTemplate::New(
          isolate, GetIdentifier));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "addBannerMessage"),
      v8::FunctionTemplate::New(
          isolate, AddBannerMessage));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "removeBannerMessage"),
      v8::FunctionTemplate::New(
          isolate, RemoveBannerMessage));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "clearBannerMessages"),
      v8::FunctionTemplate::New(
          isolate, ClearBannerMessages));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "simulateMouseClick"),
      v8::FunctionTemplate::New(
          isolate, SimulateMouseClick));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "useSimulatedMouseClicks"),
      v8::FunctionTemplate::New(
          isolate, UseSimulatedMouseClicks));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "paste"),
      v8::FunctionTemplate::New(
          isolate, Paste));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "copiedToClipboard"),
      v8::FunctionTemplate::New(
          isolate, CopiedToClipboard));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "pastedFromClipboard"),
      v8::FunctionTemplate::New(
          isolate, PastedFromClipboard));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "markFirstUseDone"),
      v8::FunctionTemplate::New(
          isolate, MarkFirstUseDone));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "enableFullScreenMode"),
      v8::FunctionTemplate::New(
          isolate, EnableFullScreenMode));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "stagePreparing"),
      v8::FunctionTemplate::New(
          isolate, StagePreparing));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "stageReady"),
      v8::FunctionTemplate::New(
          isolate, StageReady));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "containerReady"),
      v8::FunctionTemplate::New(
          isolate, ContainerReady));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "editorFocused"),
      v8::FunctionTemplate::New(
          isolate, EditorFocused));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "keepAlive"),
      v8::FunctionTemplate::New(
          isolate, KeepAlive));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "getResource"),
      v8::FunctionTemplate::New(
          isolate, GetResource));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "applyLaunchFeedback"),
      v8::FunctionTemplate::New(
          isolate, ApplyLaunchFeedback));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "addNewContentIndicator"),
      v8::FunctionTemplate::New(
          isolate, AddNewContentIndicator));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "removeNewContentIndicator"),
      v8::FunctionTemplate::New(
          isolate, RemoveNewContentIndicator));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "keyboardShow"),
      v8::FunctionTemplate::New(
          isolate, KeyboardShow));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "keyboardHide"),
      v8::FunctionTemplate::New(
          isolate, KeyboardHide));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setManualKeyboardEnabled"),
      v8::FunctionTemplate::New(
          isolate, SetManualKeyboardEnabled));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "platformBack"),
      v8::FunctionTemplate::New(
          isolate, PlatformBack));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setInputRegion"),
      v8::FunctionTemplate::New(
          isolate, SetInputRegion));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setKeyMask"),
      v8::FunctionTemplate::New(
          isolate, SetKeyMask));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setWindowProperty"),
      v8::FunctionTemplate::New(
          isolate, SetWindowProperty));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setCursor"),
      v8::FunctionTemplate::New(
          isolate, SetCursor));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setLoadErrorPolicy"),
      v8::FunctionTemplate::New(
          isolate, SetLoadErrorPolicy));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "hide"),
      v8::FunctionTemplate::New(
          isolate, Hide));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "focusOwner"),
      v8::FunctionTemplate::New(
          isolate, FocusOwner));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "focusLayer"),
      v8::FunctionTemplate::New(
          isolate, FocusLayer));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "devicePixelRatio"),
      v8::FunctionTemplate::New(
          isolate, DevicePixelRatio));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "serviceCall"),
      v8::FunctionTemplate::New(
          isolate, ServiceCall));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "cursorConstruct"),
      v8::FunctionTemplate::New(
          isolate, CursorConstruct));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "windowConstruct"),
      v8::FunctionTemplate::New(
          isolate, WindowConstruct));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "reloadInjectionData"),
      v8::FunctionTemplate::New(
          isolate, ReloadInjectionData));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "updateInjectionData"),
      v8::FunctionTemplate::New(
          isolate, UpdateInjectionData));
}

void PalmSystemInjection::PalmSystemConstruct(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::ObjectTemplate> instance_template = v8::ObjectTemplate::New();
  instance_template->SetInternalFieldCount(1);

  SetPalmSystemMethodsAndProperties(isolate, instance_template);

  v8::Local<v8::Object> instance = instance_template->NewInstance();

  PalmSystem* p = new PalmSystem("");
  p->object_.Reset(isolate, instance);
  p->object_.SetWeak(p, PalmSystem::NearObjectDeath, v8::WeakCallbackType::kParameter);
  instance->SetInternalField(0, v8::External::New(isolate, p));
  args.GetReturnValue().Set(instance);
}

void PalmSystemInjection::SetCursorMethodsAndProperties(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate>& object_template) {

  object_template->SetAccessorProperty(
      v8::String::NewFromUtf8(
          isolate, "visibility"),
      v8::FunctionTemplate::New(
          isolate, GetCursorVisibility));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "getCursorState"),
      v8::FunctionTemplate::New(
          isolate, GetCursorState));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setCursor"),
      v8::FunctionTemplate::New(
          isolate, SetCursor));
}

void PalmSystemInjection::CursorConstruct(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::ObjectTemplate> instance_template = v8::ObjectTemplate::New();
  instance_template->SetInternalFieldCount(1);

  SetCursorMethodsAndProperties(isolate, instance_template);

  v8::Local<v8::Object> instance = instance_template->NewInstance();

  PalmSystem* palmSystem = GetPalmSystemPointer(args);
  instance->SetInternalField(0, v8::External::New(isolate, palmSystem));
  args.GetReturnValue().Set(instance);
}

void PalmSystemInjection::SetWindowMethodsAndProperties(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate>& object_template) {

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setInputRegion"),
      v8::FunctionTemplate::New(
          isolate, SetInputRegion));

  object_template->Set(
      v8::String::NewFromUtf8(
          isolate, "setProperty"),
      v8::FunctionTemplate::New(
          isolate, SetWindowProperty));
}

void PalmSystemInjection::WindowConstruct(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::ObjectTemplate> instance_template = v8::ObjectTemplate::New();
  instance_template->SetInternalFieldCount(1);

  SetWindowMethodsAndProperties(isolate, instance_template);

  v8::Local<v8::Object> instance = instance_template->NewInstance();

  PalmSystem* palmSystem = GetPalmSystemPointer(args);
  instance->SetInternalField(0, v8::External::New(isolate, palmSystem));
  args.GetReturnValue().Set(instance);
}

void PalmSystemInjection::ReloadInjectionData(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  PalmSystem* palmSystem = GetPalmSystemPointer(args);
  if (palmSystem) {
    palmSystem->SetInitialisedStatus(false);
    std::string json = CallFunction("initialize");
    palmSystem->DoInitialize(json);
  }
}

void PalmSystemInjection::UpdateInjectionData(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString())
    return;

  PalmSystem* palmSystem = GetPalmSystemPointer(args);
  std::string key = *v8::String::Utf8Value(args[0]);
  std::string value = *v8::String::Utf8Value(args[1]);
 if (palmSystem)
   palmSystem->UpdateInjectionData(key, value);
}

PalmSystem* PalmSystemInjection::GetPalmSystemPointer(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Local<v8::Object> holder = args.Holder();
  v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(
      holder->GetInternalField(0));
  return static_cast<PalmSystem*>(wrap->Value());
}

std::string PalmSystemInjection::GetInjectionData(
    const std::string& name,
    const v8::FunctionCallbackInfo<v8::Value>& args) {

  PalmSystem* palmSystem = GetPalmSystemPointer(args);

  if(palmSystem) {
    // If not initialised then try to initialize
    if(!palmSystem->GetInitialisedStatus()) {
      std::string json = CallFunction("initialize");
      palmSystem->DoInitialize(json);
    }
    // If initialised then check data into cash
    if(palmSystem->GetInitialisedStatus()) {
      std::string result;
      if(palmSystem->GetInjectionData(name, result))
        return result;
    }
  }
  return CallFunction(name);
}

#if defined(USE_DYNAMIC_INJECTION_LOADING)

v8::Extension* createInjection() {
  return new PalmSystemInjection();
}

#else // defined(USE_DYNAMIC_INJECTION_LOADING)

v8::Extension* PalmSystemInjectionExtension::Get() {
  return new PalmSystemInjection();
}

#endif // defined(USE_DYNAMIC_INJECTION_LOADING)

}  // namespace extensions_v8
