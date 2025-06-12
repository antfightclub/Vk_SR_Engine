//hello
#include "vk_engine.h"

int main() 
{
    VkSREngine engine;
    
    engine.init();

    engine.run();
    
    engine.cleanup();

    return 0;
}