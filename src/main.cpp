#include "arguments/ArgumentParser.hpp"
#include "arguments/supportedArguments.hpp"
#include "arguments/defaultArguments.hpp"
#include "arguments/modeSelector.hpp"
#include "utils/randomSeed.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

bool scheduleOnly;

using namespace std;

int main(int argc, char* argv[]) {
    if(argc == 1) {
        cerr << "Usage: ./sana [OPTION] [ARG(S)] [OPTION] [ARG(S)]...\n"
       << "Try \'./sana --help\' or \'./sana -h\' for more information."
       << endl;
        exit(0);
    }

    validateAndAddArguments();

    ArgumentParser args(stringArgs, doubleArgs, boolArgs, doubleVectorArgs, stringVectorArgs);
    args.parseArgs(argc, argv, defaultArguments, true);

    //system("hostname -f >&2");

    args.writeArguments();

    if(args.doubles["-seed"] != 0) {
        setSeed(args.doubles["-seed"]);
    }
    if(args.bools["-scheduleOnly"]){
        scheduleOnly = true;
    }else{
        scheduleOnly = false;
    }
    cerr << "Seed: " << getRandomSeed() << endl;
    Mode* mode = selectMode(args);
    mode->run(args);
    delete mode;
    return 0;
}
