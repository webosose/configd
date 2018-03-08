// Copyright (c) 2014-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "FakeLunaServiceProvider.h"

#define LUNA_OLD_PREFIX "luna://"
#define LUNA_PREFIX "palm://"

typedef struct _Uri
{
    char *serviceName;
    char *category;
    char *methodName;
} _Uri;

struct Items
{
    void *m_user_data;
    LSMethod *methods;
    Items(void *_user_data, LSMethod *_methods) :
        m_user_data(_user_data),
        methods(_methods)
    {
    }

    Items(): m_user_data(NULL), methods(NULL) {}
};

struct LSHandle
{
    typedef std::map<std::string, Items> storage_type;
    storage_type items;
};

struct LSPalmService
{
    LSHandle m_public;
    LSHandle m_private;
};

struct ExtFakeLSResponse
{
    const std::string uri;
    const std::string fakeResponse;
    const std::string payload;
    const bool isSubscription;

    ExtFakeLSResponse(const std::string _uri, const std::string _fakeResponse, const bool _isSubscription,
                      const std::string _payload)
        : uri(_uri)
        , fakeResponse(_fakeResponse)
        , isSubscription(_isSubscription)
        , payload(_payload)
    {
    }
    bool operator< (const ExtFakeLSResponse &other)
    {
        return uri.compare(other.uri);
    }

};

std::map<std::string, LSPalmService * > g_service_storage;
std::set<std::shared_ptr<const ExtFakeLSResponse>> g_extFakeLSResponse;

/*
struct FindProcByPId {
    FindProcByPId( const std::string& p ) : toFind(p) { }
    bool operator() (const std::shared_ptr<const ProcessBase>& other){ return toFind == other->processId(); }
    const std::string& toFind;
};*/

struct findLSResponse
{
    const std::string _uri;
    const std::string _payload;

    findLSResponse(const std::string &_uri, const std::string &_payload)
        : _uri(_uri), _payload(_payload) {}

    bool operator()(const std::shared_ptr<const ExtFakeLSResponse> &other)
    {
        if ((*other).payload.empty())
        {
            if ((*other).uri.compare(_uri) == 0)
            {
                return true;
            }
        }
        else
        {
            if (((*other).uri.compare(_uri) == 0) &&
                    ((*other).payload.compare(_payload) == 0))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        return false;
    }
};

class MESSAGE_REQ
{
public:
    std::string uri;
    std::string payload;
    LSFilterFunc callback;
    void *user_data;

    MESSAGE_REQ(): callback(NULL), user_data(NULL) {}
    MESSAGE_REQ(std::string _uri, std::string _payload, LSFilterFunc _callback, void *_user_data):
        uri(_uri), payload(_payload), callback(_callback), user_data(_user_data) {}
};

std::map<LSMessageToken, MESSAGE_REQ > g_fakeLSRequests;
std::map<std::string, MESSAGE_REQ > g_fakeLSResponses;

void _UriFree(_Uri *luri)
{
    if (NULL == luri)
    {
        return;
    }

    g_free(luri->serviceName);
    g_free(luri->category);
    g_free(luri->methodName);

    g_free(luri);
}

_Uri *_UriParse(const char *uri)
{
    _Uri *luri = NULL;
    const char *uri_p;
    const char *first_slash;
    int service_name_len;

    uri_p = uri;

    if (g_str_has_prefix(uri, LUNA_PREFIX))
    {
        uri_p += strlen(LUNA_PREFIX);
    }
    else if (g_str_has_prefix(uri, LUNA_OLD_PREFIX))
    {
        uri_p += strlen(LUNA_OLD_PREFIX);
    }
    else
    {
        return 0;
    }

    first_slash = strchr(uri_p, '/');

    if (!first_slash)
    {
        return 0;
    }

    luri = g_new0(_Uri, 1);

    if (!luri)
    {
        return 0;
    }

    service_name_len = first_slash - uri_p;
    luri->serviceName = g_strndup(uri_p, service_name_len);
    uri_p += service_name_len;

    if (!luri->serviceName)
    {
        return 0;
    }

    luri->category = g_path_get_dirname(uri_p);
    luri->methodName = g_path_get_basename(uri_p);

    if (!luri->category || !luri->methodName)
    {
        return 0;
    }

    return luri;
}

std::string _UriRemovePrefix(const std::string &uri)
{
    std::string parseUri = uri;

    std::size_t found = std::string::npos;

    found = parseUri.find(LUNA_OLD_PREFIX);

    if (found != std::string::npos)
    {
        parseUri = parseUri.substr(strlen(LUNA_OLD_PREFIX), parseUri.length());
    }
    else
    {
        found = parseUri.find(LUNA_PREFIX);

        if (found != std::string::npos)
        {
            parseUri = parseUri.substr(strlen(LUNA_PREFIX), parseUri.length());
        }
        else
        {
            return NULL;
        }
    }

    return parseUri;
}


void FakeLSReset()
{
    std::map<std::string, LSPalmService * >::iterator pIt = g_service_storage.begin();

    for (; pIt != g_service_storage.end(); ++pIt)
    {
        delete pIt->second;
    }

    g_service_storage.clear();
    g_extFakeLSResponse.clear();
    g_fakeLSRequests.clear();
    g_fakeLSResponses.clear();
}

bool FakeLSSetExtResponse(const std::string &uriApi, const std::string &fakeResponse, const bool isSubscription,
                          const std::string &payload)
{
    std::string rUri = _UriRemovePrefix(uriApi);

    if (rUri.empty())
    {
        return false;
    }

    std::shared_ptr<const ExtFakeLSResponse> newResponse(new ExtFakeLSResponse(rUri, fakeResponse, isSubscription,
            payload));
    auto it = std::find_if(g_extFakeLSResponse.begin(), g_extFakeLSResponse.end(),
                           [ = ](const std::shared_ptr<const ExtFakeLSResponse> v) -> bool
    { return (v->uri == rUri); });

    if (it != g_extFakeLSResponse.end())
    {
        g_extFakeLSResponse.erase(it);
    }

    g_extFakeLSResponse.insert(newResponse);

    return true;
}

/*
 * Simulate LSCall() as did in g_main_loop in real LS2
 *
 * The call is processed in two ways.
 *
 * If the call is to internal API which is registered with LSRegisterPalmService(), then
 *  -> The message is saved to be used in LSMessageReply() later and the internal handler function is searched and called with the message.
 * If the call is to external API which is NOT registered with LSRegisterPalmService(), then
 *  -> The callback function is immediately called with the pre-set return value. You can set the return value for external call with FakeLSSetExtResponse()
 *
 **/
void FakeLSRunLSCall()
{
    for (auto it = g_fakeLSRequests.begin(); it != g_fakeLSRequests.end();)
    {
        const MESSAGE_REQ &msgReq = it->second;
        _Uri *luri = _UriParse(msgReq.uri.c_str());
        LSPalmService *record = g_service_storage[luri->serviceName];
        bool msgHandled = false;

        //internal call
        if (record)
        {
            void *data = 0;
            LSMethodFunction pfn =  TestUtil::get_cb_pointer(luri->methodName, record->m_public.items[luri->category].methods);
            data = record->m_public.items[luri->category].m_user_data;

            if (!pfn)
            {
                pfn =  TestUtil::get_cb_pointer(luri->methodName, record->m_private.items[luri->category].methods);
                data = record->m_private.items[luri->category].m_user_data;
            }

            // The msg is saved to be used in LSMessageReply()
            g_fakeLSResponses[luri->methodName] = it->second;

            if (pfn)
            {
                LSMessage *pmsg = TestUtil::create_message(NULL, luri->category, luri->methodName, msgReq.payload.c_str(), it->first);
                pfn(NULL, pmsg, data);
                LSMessageUnref(pmsg);
            }

            msgHandled = true;
        }
        //external call
        else if (msgReq.callback)
        {

            auto rspIt = std::find_if(g_extFakeLSResponse.begin(), g_extFakeLSResponse.end(),
                                      findLSResponse(_UriRemovePrefix(msgReq.uri), msgReq.payload));

            if (g_extFakeLSResponse.end() != rspIt)
            {
                // simulation API of external Service
                LSMessage *pmsg = TestUtil::create_message(NULL, luri->category, luri->methodName, (*rspIt)->fakeResponse.c_str(),
                                  it->first);
                msgReq.callback(NULL, pmsg, msgReq.user_data);
                LSMessageUnref(pmsg);
                msgHandled = true;
                g_extFakeLSResponse.erase(rspIt);

                if ((*rspIt)->isSubscription)
                {
                    msgHandled = false;
                }
            }
        }

        if (msgHandled)
        {
            g_fakeLSRequests.erase(it++);    // msg handled. remove it from the requests queue.
        }
        else
        {
            it++;    // unhandled message. leave it to be handled later.
        }

        _UriFree(luri);
    }
}


// In GCC (not supported in C++), we can rely on how the linker works.
// during name resolution the linker will favor local implementation
bool LSErrorInit(LSError *error)
{
    return true;
}

void LSErrorFree(LSError *error)
{
}

bool LSErrorIsSet(LSError *lserror)
{
    return true;
}

void LSErrorPrint(LSError *lserror, FILE *out)
{
}

bool LSRegister(const char *name, LSHandle **sh,
                LSError *lserror)
{
    return true;
}

bool LSRegisterPubPriv(const char *name, LSHandle **sh,
                       bool public_bus,
                       LSError *lserror)
{
    return true;
}

bool LSSetDisconnectHandler(LSHandle *sh, LSDisconnectHandler disconnect_handler,
                            void *user_data, LSError *lserror)
{
    return true;
}

bool LSRegisterCategory(LSHandle *sh, const char *category,
                        LSMethod      *methods,
                        LSSignal      *langis,
                        LSProperty    *properties, LSError *lserror)
{
    return true;
}

bool LSRegisterCategoryAppend(LSHandle *sh, const char *category,
                              LSMethod      *methods,
                              LSSignal      *langis,
                              LSError *lserror)
{

    return true;
}

bool LSCategorySetData(LSHandle *sh, const char *category,
                       void *user_data, LSError *lserror)
{
    return true;
}

bool LSUnregister(LSHandle *service, LSError *lserror)
{
    return true;
}

const char *LSHandleGetName(LSHandle *sh)
{
    return 0;
}

bool LSRegisterPalmService(const char *name,
                           LSPalmService **ret_palm_service,
                           LSError *lserror)
{
    *ret_palm_service = new LSPalmService();
    g_service_storage[name] = *ret_palm_service;
    return true;
}

bool LSUnregisterPalmService(LSPalmService *psh, LSError *lserror)
{


    std::map<std::string, LSPalmService * >::iterator it = g_service_storage.begin();

    for (; it != g_service_storage.end(); ++ it)
    {
        if (it->second == psh)
        {
            g_service_storage.erase(it);
            break;
        }
    }

    delete psh;
    return true;
}

bool LSPalmServiceRegisterCategory(LSPalmService *psh,
                                   const char *category,
                                   LSMethod *methods_public, LSMethod *methods_private,
                                   LSSignal *langis, void *category_user_data, LSError *lserror)
{
    Items prv_item(category_user_data, methods_private);
    psh->m_private.items[category] = prv_item;

    Items pub_item(category_user_data, methods_public);
    psh->m_public.items[category] = pub_item;

    return true;
}

LSHandle *LSPalmServiceGetPrivateConnection(LSPalmService *psh)
{
    return &psh->m_private;
}

LSHandle *LSPalmServiceGetPublicConnection(LSPalmService *psh)
{
    return &psh->m_public;
}

bool LSPushRole(LSHandle *sh, const char *role_path, LSError *lserror)
{
    return true;
}

bool LSPushRolePalmService(LSPalmService *psh, const char *role_path, LSError *lserror)
{
    return true;
}

LSHandle *LSMessageGetConnection(LSMessage *message)
{
    return 0;
}

bool LSMessageIsPublic(LSPalmService *psh, LSMessage *message)
{
    return true;
}

void LSMessageRef(LSMessage *message)
{
    g_atomic_int_inc(&message->ref);
}

void LSMessageUnref(LSMessage *message)
{
    if (g_atomic_int_dec_and_test(&message->ref))
    {
        delete message;
    }
}

bool LSMessagePrint(LSMessage *lmsg, FILE *out)
{
    return true;
}

bool LSMessageIsHubErrorMessage(LSMessage *message)
{
    return true;
}

const char *LSMessageGetUniqueToken(LSMessage *message)
{
    return 0;
}

const char *LSMessageGetKind(LSMessage *message)
{
    return 0;
}

const char *LSMessageGetApplicationID(LSMessage */*message*/)
{
    return "com.yourdomain.helloworld 1234";
}

const char *LSMessageGetSender(LSMessage *message)
{
    return 0;
}

const char *LSMessageGetSenderServiceName(LSMessage *message)
{
    return 0;
}

const char *LSMessageGetCategory(LSMessage *message)
{
    return TestUtil::message_get_category(message);
}

const char *LSMessageGetMethod(LSMessage *message)
{
    return TestUtil::message_get_method(message);
}

const char *LSMessageGetPayload(LSMessage *message)
{
    return TestUtil::message_get_payload(message);
}

bool LSMessageIsSubscription(LSMessage *lsmgs)
{
    if (NULL == lsmgs)
    {
        return false;
    }

    //    pbnjson::JValue payload = JUtil::parse(LSMessageGetPayload(lsmgs), "");
    //    if(payload.hasKey("subscribe"))
    //        return payload["subscribe"].asBool();

    return false;
}

LSMessageToken LSMessageGetToken(LSMessage *call)
{
    //not identical with the luna-service
    return call->responseToken;
}

LSMessageToken LSMessageGetResponseToken(LSMessage *reply)
{
    return reply->responseToken;
}

bool LSMessageRespond(LSMessage *message, const char *replyPayload,
                      LSError *lserror)
{
    if (!LSMessageReply(NULL, message, replyPayload, lserror))
    {
        LSErrorPrint(lserror, stderr);
    }

    return true;
}

bool LSMessageReply(LSHandle *sh, LSMessage *lsmsg, const char *replyPayload,
                    LSError *lserror)
{
    if (!lsmsg || lsmsg->method.empty())
    {
        return true;
    }

    // Find the original internal message by method name and invoke the callback function.
    auto it = g_fakeLSResponses.find(lsmsg->method);

    if (g_fakeLSResponses.end() != it)
    {
        const MESSAGE_REQ &msgReq = it->second;
        lsmsg->payload = replyPayload;
        msgReq.callback(sh, lsmsg, msgReq.user_data);
        g_fakeLSResponses.erase(it);
    }

    return true;
}

bool LSGmainAttach(LSHandle *sh, GMainLoop *mainLoop, LSError *lserror)
{
    return true;
}

bool LSGmainAttachPalmService(LSPalmService *psh,
                              GMainLoop *mainLoop, LSError *lserror)
{
    return true;
}

bool LSGmainDetach(LSHandle *sh, LSError *lserror)
{
    return true;
}

bool LSGmainSetPriority(LSHandle *sh, int priority, LSError *lserror)
{
    return true;
}

bool LSGmainSetPriorityPalmService(LSPalmService *psh, int priority, LSError *lserror)
{
    return true;
}

//! in most cases we are using only one request.
bool LSCall(LSHandle *sh, const char *uri, const char *payload,
            LSFilterFunc callback, void *user_data,
            LSMessageToken *ret_token, LSError *lserror)
{
    static LSMessageToken TokenSeed = 0;
    ++TokenSeed;

    if (ret_token)
    {
        *ret_token = TokenSeed;
    }

    g_fakeLSRequests[TokenSeed] = MESSAGE_REQ(uri, payload, callback, user_data);

    return true;
}

bool LSCallOneReply(LSHandle *sh, const char *uri, const char *payload,
                    LSFilterFunc callback, void *ctx,
                    LSMessageToken *ret_token, LSError *lserror)
{
    return LSCall(sh, uri, payload, callback, ctx, ret_token, lserror);
}

bool LSCallFromApplication(LSHandle *sh, const char *uri, const char *payload,
                           const char *applicationID,
                           LSFilterFunc callback, void *ctx,
                           LSMessageToken *ret_token, LSError *lserror)
{
    return true;
}

bool LSCallFromApplicationOneReply(
    LSHandle *sh, const char *uri, const char *payload,
    const char *applicationID,
    LSFilterFunc callback, void *ctx,
    LSMessageToken *ret_token, LSError *lserror)
{
    return true;
}

bool LSCallCancel(LSHandle *sh, LSMessageToken token, LSError *lserror)
{
    return true;
}

bool LSSubscriptionProcess(LSHandle *sh, LSMessage *message, bool *subscribed,
                           LSError *lserror)
{
    return true;
}

bool LSSubscriptionSetCancelFunction(LSHandle *sh,
                                     LSFilterFunc cancelFunction,
                                     void *ctx, LSError *lserror)
{
    return true;
}

bool LSSubscriptionAdd(LSHandle *sh, const char *key,
                       LSMessage *message, LSError *lserror)
{
    return true;
}

bool LSSubscriptionAcquire(LSHandle *sh, const char *key,
                           LSSubscriptionIter **ret_iter, LSError *lserror)
{
    return true;
}

void LSSubscriptionRelease(LSSubscriptionIter *iter)
{
}

bool LSSubscriptionHasNext(LSSubscriptionIter *iter)
{
    return true;
}

LSMessage *LSSubscriptionNext(LSSubscriptionIter *iter)
{
    return 0;
}

void LSSubscriptionRemove(LSSubscriptionIter *iter)
{
}

bool LSSubscriptionReply(LSHandle *sh, const char *key,
                         const char *payload, LSError *lserror)
{
    //TODO : implement fake subscribe
    return true;
}

bool LSSubscriptionRespond(LSPalmService *psh, const char *key,
                           const char *payload, LSError *lserror)
{
    return true;
}

bool LSSubscriptionPost(LSHandle *sh, const char *category,
                        const char *method,
                        const char *payload, LSError *lserror)
{
    return true;
}

bool LSSignalSend(LSHandle *sh, const char *uri, const char *payload,
                  LSError *lserror)
{
    return true;
}

bool LSSignalSendNoTypecheck(LSHandle *sh,
                             const char *uri, const char *payload, LSError *lserror)
{
    return true;
}

bool LSSignalCall(LSHandle *sh,
                  const char *category, const char *methodName,
                  LSFilterFunc filterFunc, void *ctx,
                  LSMessageToken *ret_token,
                  LSError *lserror)
{
    return true;
}

bool LSSignalCallCancel(LSHandle *sh, LSMessageToken token, LSError *lserror)
{
    return true;
}

bool LSRegisterServerStatus(LSHandle *sh, const char *serviceName,
                            LSServerStatusFunc func, void *ctx, LSError *lserror)
{
    return true;
}

