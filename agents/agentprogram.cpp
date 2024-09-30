// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agentprogram.h"

#include "../genlib/pafmath.h"
#include "../genlib/stringutils.h"

#include <fstream>

AgentProgram::AgentProgram()
    : selType(SEL_LOS), steps(3), vbin(7), destinationDirected(false), losSqrd(false) {}

void AgentProgram::mutate() {
    // do mutate rule order occassionally:
    if (pafmath::pafrand() % 20 == 0) {
        // rule order relies on putting rules into slots:
        for (int i = 0; i < 4; i++) {
            ruleOrder[i] = -1;
        }
        for (int j = 0; j < 4; j++) {
            int choice = pafmath::pafrand() % (4 - j);
            for (int k = 0; k < choice + 1; k++) {
                if (ruleOrder[k] != -1) {
                    choice++;
                }
            }
            ruleOrder[choice] = j;
        }
    }
    // mutate the rule threshold / probabilities
    for (int i = 0; i < 4; i++) {
        if (pafmath::pafrand() % 20 == 0) { // 5% mutation rate
            ruleThreshold[i] = float(pafmath::prandom() * 100.0);
        }
        if (pafmath::pafrand() % 20 == 0) { // 5% mutation rate
            ruleProbability[i] = float(pafmath::prandom());
        }
    }
}

AgentProgram crossover(const AgentProgram &progA, const AgentProgram &progB) {
    AgentProgram child;

    // either one rule priority order or the other (don't try to mix!)
    if (pafmath::pafrand() % 2) {
        for (int i = 0; i < 4; i++) {
            child.ruleOrder[i] = progA.ruleOrder[i];
        }
    } else {
        for (int i = 0; i < 4; i++) {
            child.ruleOrder[i] = progB.ruleOrder[i];
        }
    }
    // for each rule, either one rule threshold / probability or the other:
    for (int i = 0; i < 4; i++) {
        if (pafmath::pafrand() % 2) {
            child.ruleThreshold[i] = progA.ruleThreshold[i];
        } else {
            child.ruleThreshold[i] = progB.ruleThreshold[i];
        }
        if (pafmath::pafrand() % 2) {
            child.ruleProbability[i] = progA.ruleProbability[i];
        } else {
            child.ruleProbability[i] = progB.ruleProbability[i];
        }
    }

    return child;
}

// TODO: Expose this functionality to the UIs
void AgentProgram::save(const std::string &filename) {
    // standard ascii:
    std::ofstream file(filename.c_str());

    file << "Destination selection: ";
    switch (selType) {
    case SEL_STANDARD:
        file << "Standard" << std::endl;
        break;
    case SEL_LENGTH:
        file << "Gibsonian Length" << std::endl;
        break;
    case SEL_OPTIC_FLOW:
        file << "Gibsonian Optic Flow" << std::endl;
        break;
    case SEL_COMPARATIVE_LENGTH:
        file << "Gibsonian Comparative Length" << std::endl;
        break;
    case SEL_COMPARATIVE_OPTIC_FLOW:
        file << "Gibsonian Comparative Optic Flow" << std::endl;
        break;
    default:
        file << "Unknown" << std::endl;
    }

    file << "Steps: " << steps << std::endl;
    file << "Bins: " << ((vbin == -1) ? 32 : vbin * 2 + 1) << std::endl;
    file << "Rule order: " << ruleOrder[0] << " " << ruleOrder[1] << " " << ruleOrder[2] << " "
         << ruleOrder[3] << std::endl;

    for (int i = 0; i < 4; i++) {
        file << "Rule " << i << " (Bin -" << 1 + (i * 2) << "/+" << 1 + (i * 2) << ")" << std::endl;
        file << "Threshold: " << ruleThreshold[i] << std::endl;
        file << "Turn Probability: " << ruleProbability[i] << std::endl;
    }

    file << "Fitness: " << fitness << std::endl;
}

bool AgentProgram::open(const std::string &filename) {
    // standard ascii:
    std::ifstream file(filename.c_str());

    std::string line;
    file >> line;
    if (!line.empty()) {
        dXstring::toLower(line);
        if (line.substr(0, 22) != "destination selection:") {
            return false;
        } else {
            std::string method = line.substr(22);
            dXstring::ltrim(method);
            if (!method.empty()) {
                if (method == "standard") {
                    selType = SEL_STANDARD;
                } else if (method == "gibsonian length") {
                    selType = SEL_LENGTH;
                } else if (method == "gibsonian optic flow") {
                    selType = SEL_OPTIC_FLOW;
                } else if (method == "gibsonian comparative length") {
                    selType = SEL_COMPARATIVE_LENGTH;
                } else if (method == "gibsonian comparative optic flow") {
                    selType = SEL_COMPARATIVE_OPTIC_FLOW;
                }
                file >> line;
            } else {
                return false;
            }
        }
    } else {
        return false;
    }

    bool foundsteps = false;
    bool foundbins = false;

    if (!line.empty()) {
        dXstring::toLower(line);
        if (line.substr(0, 6) == "steps:") {
            std::string steps = line.substr(6);
            dXstring::ltrim(steps);
            steps = stoi(steps);
            file >> line;
            foundsteps = true;
        }
    } else {
        return false;
    }

    if (!line.empty()) {
        dXstring::toLower(line);
        if (line.substr(0, 5) == "bins:") {
            std::string bins = line.substr(6);
            dXstring::ltrim(bins);
            int binx = stoi(bins);
            if (binx == 32) {
                vbin = -1;
            } else {
                vbin = (atoi(bins.c_str()) - 1) / 2;
            }
            file >> line;
            foundbins = true;
        }
    }

    if (selType == SEL_STANDARD) {
        if (foundbins && foundsteps) {
            return true;
        } else {
            return false;
        }
    }

    if (!line.empty()) {
        dXstring::toLower(line);
        if (line.substr(0, 11) == "rule order:") {
            std::string ruleorder = line.substr(11);
            dXstring::ltrim(ruleorder);
            auto orders = dXstring::split(ruleorder, ' ');
            if (orders.size() != 4) {
                return false;
            }
            for (int i = 0; i < 4; i++) {
                ruleOrder[i] = stoi(orders[i]);
            }
            file >> line;
        } else {
            return false;
        }
    } else {
        return false;
    }
    for (int i = 0; i < 4; i++) {
        if (!line.empty()) {
            dXstring::toLower(line);
            if (line.substr(0, 4) == "rule") {
                file >> line;
            }
            dXstring::toLower(line);
            if (line.substr(0, 10) == "threshold:") {
                auto threshold = line.substr(10);
                dXstring::ltrim(threshold);
                ruleThreshold[i] = stof(threshold);
                file >> line;
            } else {
                return false;
            }
            dXstring::toLower(line);
            if (line.substr(0, 17) == "turn probability:") {
                auto prob = line.substr(17);
                dXstring::ltrim(prob);
                ruleProbability[i] = stof(prob);
                file >> line;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}
