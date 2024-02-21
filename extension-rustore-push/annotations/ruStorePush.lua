---@diagnostic disable: lowercase-global
---@diagnostic disable: missing-return
---@diagnostic disable: duplicate-doc-param
---@diagnostic disable: duplicate-set-field
---@diagnostic disable: unused-local

---@class ruStorePush
ruStorePush = {}

---AID is not available
---@type number
ruStorePush.CLIENT_AID_NOT_AVAILABLE = nil
---GAID
---@type number
ruStorePush.CLIENT_GAID = nil
---OAID
---@type number
ruStorePush.CLIENT_OAID = nil


---Set on token listener
---@param callback fun(self:object, token:string, error:table) function to call when the token is received
---self
---object The script instance
---token
---string Push token
---error
---table Error. Contains the fields:
---
---
---string error: error if the token is not received
function ruStorePush.set_on_token(callback) end

---Set on message listener
---@param callback fun(self:object, message:string, activated:boolean, from:string) function to call when the message is received
---self
---object The script instance
---message
---string Push message
---activated
---boolean If the application was activated via the notification.
---from
---string Push comming from, used for topics
function ruStorePush.set_on_message(callback) end

---Set client id callback
---@param callback fun(self:object): aid:string, aid_type:number function to call when the client id is received
---self
---object The script instance
function ruStorePush.set_client_id_callback(callback) end

---Delete token
---@param callback fun(self:object, error:table) function to call when the token is deleted
---self
---object The script instance
---error
---table Error. Contains the fields:
---
---
---string error: error if the token is not deleted
function ruStorePush.delete_token(callback) end

---Topic subscribe
---@param topic string topic name.
---@param callback fun(self:object, error:table) function to call with result
---self
---object The script instance
---error
---table Error. Contains the fields:
---
---
---string error: error if subscription failed
function ruStorePush.topic_subscribe(topic, callback) end

---Topic unsubscribe
---@param topic string topic name.
---@param callback fun(self:object, error:table) function to call with result
---self
---object The script instance
---error
---table Error. Contains the fields:
---
---
---string error: error if unsubscription failed
function ruStorePush.topic_unsubscribe(topic, callback) end

return ruStorePush