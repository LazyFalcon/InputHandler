#include "InputHandler.hpp"
#include "Logging.hpp"

static bool debug = true;
static bool running = true;

namespace InputHandler {

const int LMB = 501;
const int MMB = 502;
const int RMB = 503;
const int SCROLL_UP = 504;
const int SCROLL_DOWN = 505;
const int HOLD_KEY = 666;
int currentMods;


const std::map<std::string, int> stringToMod = {
    { "shift", GLFW_MOD_SHIFT },
    { "ctrl", GLFW_MOD_CONTROL },
    { "alt", GLFW_MOD_ALT },
    { "super", GLFW_MOD_SUPER },
};

const std::map<std::string, int> stringToKey = {
    { "space", GLFW_KEY_SPACE },
    { "esc", GLFW_KEY_ESCAPE },
    { "enter", GLFW_KEY_ENTER },
    { "tab", GLFW_KEY_TAB },
    { "backspace", GLFW_KEY_BACKSPACE },
    { "insert", GLFW_KEY_INSERT },
    { "delete", GLFW_KEY_DELETE },
    { "right", GLFW_KEY_RIGHT },
    { "left", GLFW_KEY_LEFT },
    { "down", GLFW_KEY_DOWN },
    { "up", GLFW_KEY_UP },
    { "pageUp", GLFW_KEY_PAGE_UP },
    { "pageDown", GLFW_KEY_PAGE_DOWN },
    { "home", GLFW_KEY_HOME },
    { "end", GLFW_KEY_END },
    { "capsLock", GLFW_KEY_CAPS_LOCK },
    { "scrollLock", GLFW_KEY_SCROLL_LOCK },
    { "printScreen", GLFW_KEY_PRINT_SCREEN },
    { "pause", GLFW_KEY_PAUSE },
    { "f1", GLFW_KEY_F1 },
    { "f2", GLFW_KEY_F2 },
    { "f3", GLFW_KEY_F3 },
    { "f4", GLFW_KEY_F4 },
    { "f5", GLFW_KEY_F5 },
    { "f6", GLFW_KEY_F6 },
    { "f7", GLFW_KEY_F7 },
    { "f8", GLFW_KEY_F8 },
    { "f9", GLFW_KEY_F9 },
    { "f10", GLFW_KEY_F10 },
    { "f11", GLFW_KEY_F11 },
    { "f12", GLFW_KEY_F12 },
    { "menu", GLFW_KEY_MENU },
    { "shift", GLFW_KEY_LEFT_SHIFT },
    { "ctrl", GLFW_KEY_LEFT_CONTROL },
    { "alt", GLFW_KEY_LEFT_ALT },
    { "super", GLFW_KEY_LEFT_SUPER },
    { "hold", HOLD_KEY},
    { "scrollUp", SCROLL_UP},
    { "scrollDown", SCROLL_DOWN},
    { "LMB", LMB},
    { "MMB", MMB},
    { "RMB", RMB},
    { "minus", int('-')},
};

/// keys:function
std::pair<std::string, std::string> splitToFunctionAndKeys(const std::string &str){
    // str.erase( remove(str.begin(), str.end(),' '), str.end() );
    int a=0;
    for(int i=0; i<str.size(); i++){
        if(str[i] == ':'){
            return std::make_pair(str.substr(i+1), str.substr(0, i));
        }
    }
    return {};
}
std::vector<std::string> splitToKeys(std::string str){
    if(str.size() == 1) return {str};
    if(str.back() == '-') str.replace(str.size()-1, 1, "minus");

    int a=0;
    std::vector<std::string> values;
    for(int i=0; i<str.size(); i++){
        if(str[i] == '-'){
            values.push_back(str.substr(a, i-a));
            a = i+1;
        }
    }
    values.push_back(str.substr(a));
    return values;
}
struct KeyActionMode
{
    int key;
    int action;
    int modifier;
};
KeyActionMode parseKeyBinding(const std::string &str){
    KeyActionMode out {};
    std::vector<std::string> values = splitToKeys(str);
    // log(str);
    // for(auto &it : values) log("\t", it);
    if(values.front() == "hold"){
        out.action = GLFW_REPEAT;
        values = std::vector<std::string>(values.begin()+1, values.end());
    }
    else out.action = GLFW_PRESS;

    for(int i=0; i<values.size()-1; i++){
        if(stringToMod.count(values[i])){
            out.modifier |= stringToMod.at(values[i]);
        }
        else {
            error("No modifier key:", values[i]);
        }
    }
    if(stringToKey.count(values.back()))
        out.key = stringToKey.at(values.back());
    else {
        out.key = int(values.back()[0]);
        // log("[Warning] unknown key value:", values.back());
    }
    if(out.key >= 'a' and out.key <= 'z') out.key -= 'a' - 'A';
    // log("\t", out.key);
    return out;
}

inline u32 hashInput(int k, int a, int m){
    if(k > 256){
        switch(k){
            case GLFW_KEY_KP_ENTER: { k = GLFW_KEY_ENTER ; break; }
            case GLFW_KEY_KP_DIVIDE : { k = '/'; break; }
            case GLFW_KEY_KP_MULTIPLY: { k = '*'; break; }
            case GLFW_KEY_KP_SUBTRACT : { k = '-'; break; }
            case GLFW_KEY_KP_ADD : { k = '+'; break; }
            case GLFW_KEY_KP_EQUAL : { k = '='; break; }
            case GLFW_KEY_RIGHT_SHIFT : { k = GLFW_KEY_LEFT_SHIFT; break; }
            case GLFW_KEY_RIGHT_CONTROL : { k = GLFW_KEY_LEFT_CONTROL; break; }
            case GLFW_KEY_RIGHT_ALT : { k = GLFW_KEY_LEFT_ALT; break; }
            case GLFW_KEY_RIGHT_SUPER : { k = GLFW_KEY_LEFT_SUPER; break; }
            default:{
                if(k >= GLFW_KEY_KP_0 and k <= GLFW_KEY_KP_9) k -= GLFW_KEY_KP_0 + '0';
            }
        }
        /// in case we want only shift pressed and behave as key not mod
        if(k == GLFW_KEY_LEFT_SHIFT and m & GLFW_MOD_SHIFT) m = 0;
        if(k == GLFW_KEY_LEFT_CONTROL and m & GLFW_MOD_CONTROL) m = 0;
        if(k == GLFW_KEY_LEFT_ALT and m & GLFW_MOD_ALT) m = 0;
        if(k == GLFW_KEY_LEFT_SUPER and m & GLFW_MOD_SUPER) m = 0;
    }

    /// m is 4bits, a is 2bits, k is at least 9bits
    return u32( k<<6 | a <<4 | m );
}
inline u32 hashInput(KeyActionMode keys){
    return hashInput(keys.key, keys.action, keys.modifier);
}
void scrollCallback(double dx, double dy){
    // if(debug) log(__FUNCTION__, "dx:", dx, "dy:", dy);
    if(dy > 0) execute(SCROLL_UP, GLFW_PRESS, currentMods);
    if(dy < 0) execute(SCROLL_DOWN, GLFW_PRESS, currentMods);
}
void keyCallback(int key, int action, int mods){
    // if(debug) log(__FUNCTION__, "key:", key, "action:", action, "mods:", mods);
    currentMods = mods;
    execute(key, action, mods);
}
void mouseButtonCallback(int button, int action, int mods){
    // if(debug) log(__FUNCTION__, "button:", button, "action:", action, "mods:", mods);

    currentMods = mods;
    switch(button){
        case GLFW_MOUSE_BUTTON_LEFT: { button = LMB; break; }
        case GLFW_MOUSE_BUTTON_RIGHT: { button = RMB; break; }
        case GLFW_MOUSE_BUTTON_MIDDLE: { button = MMB; break; }
    }
    execute(button, action, mods);
}
struct InputEvent
{
    std::string name;
    std::function<void(void)> func;
    void operator () (){
        if(func) {
            log("function", name);
            func();
        }
    }
};

class InputHandlerContextBindingContainer
{
public:
    InputHandlerContextBindingContainer(const std::string &name) : name(name){}
    void emplace(int k, int a, int m, const std::string &internalName, Lambda func){
        auto hashed = hashInput(k, a, m);
        if(map.count(hashed)){
            error(internalName, "is already defined in this handler:", k,a,m);
            return;
        }
        map.emplace(hashed, InputEvent{internalName, func});
    }
    void erase(const std::string &internalName){
        for(auto it=map.begin(); it != map.end(); it++){
            if(it->second.name == internalName)
                map.erase(it);
        }
    }
    void unset(const std::string &str){
        auto keys = parseKeyBinding(str);
        map.erase(hashInput(keys));
        keys.action = GLFW_RELEASE;
        map.erase(hashInput(keys));
    }
    bool execute(int k, int a, int m){
        // log(k,a,m);
        auto it = map.find(hashInput(k,a,m));
        if(it != map.end()){
            it->second();
            return true;
        }
        return false;
    }
    std::string name;
    bool active {false};
private:
    std::map<u32, InputEvent> map;
};

std::deque<Context*> stackOfContext;
std::multimap<std::string, std::string> functionAndKeyBindings;

void forEachBinding(const std::string &functionName, std::function<void(const std::string&)> fun){
    auto keys = functionAndKeyBindings.equal_range(functionName);
    for (auto it = keys.first; it != keys.second; ++it)
        fun(it->second);
}
void registerKeyCombination(const std::string &str){
    auto funcAndKeys = splitToFunctionAndKeys(str);
    functionAndKeyBindings.emplace(funcAndKeys.first, funcAndKeys.second);
}
void activate(Context* context){
    stackOfContext.push_front(context);
}
void deactivate(Context* context){
    std::remove_if(std::begin(stackOfContext), std::end(stackOfContext), [&context](const Context* it){ return it==context; });
}

void execute(int k, int a, int m){
    for(auto it = stackOfContext.begin(); it != stackOfContext.end(); it++){
        (*it)->contextImpl->execute(k, a, m);
        if((*it)->consumeInput == CONSUME_ALL) break;
    }
}

Context::Context(std::string contextName, ConsumeInput consumeInput) : contextName(contextName), consumeInput(consumeInput), contextImpl(std::make_shared<InputHandlerContextBindingContainer>(contextName)){
}
Context::~Context(){
    deactivate();
}
void Context::setBinding(const std::string &functionName, Lambda onEnter, Lambda onExit){
    InputHandler::forEachBinding(functionName, [&](const std::string &binding){
        setBinding(binding, functionName, onEnter, onExit);
    });
}
void Context::setBinding(const std::string &binding, const std::string &name, Lambda onEnter, Lambda onExit){
    auto keys = parseKeyBinding(binding);
    if(onEnter){
        // log(binding, keys.key, keys.action, keys.modifier);
        contextImpl->emplace(keys.key, keys.action, keys.modifier, name, onEnter);
    }
    keys.action = GLFW_RELEASE;
    if(onExit){
        contextImpl->emplace(keys.key, keys.action, keys.modifier, name, onExit);
    }
}

void Context::execute(int k, int a, int m){
    contextImpl->execute(k, a, m);
}
void Context::activate(){
    InputHandler::activate(this);
}
void Context::deactivate(){
    InputHandler::deactivate(this);
}

}