#pragma once

#include <string>

class Setup {
   private:
    static const std::string dataPath;
    static const std::string files[];

   public:
    static void initializeFiles();
};
