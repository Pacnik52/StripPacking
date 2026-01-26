#include <random>
#include <SFML/Graphics.hpp>
#include "BinDrawer.h"
#include "../utils.h"
#include <set>
#include <string>
#include <algorithm>
#include <climits>
#include <iostream>
#include "../bin/BinpackData.h"
#include "../startegy/BinpackConstructionHeuristic.h"

namespace binpack {
    void BinDrawer::print_solutions(std::vector<BinpackData>& datasets, BinpackConstructionHeuristic<nnutils::FFN>& heuristic, const std::string& outputDir, bool draw_all_solutions) {
        if (!std::filesystem::exists(outputDir)) {
            std::filesystem::create_directory(outputDir);
        }

        std::cout << "\nGenerating final solutions and images..." << std::endl;

        double totalFF = 0;
        for (size_t i = 0; i < datasets.size(); ++i) {
            // Pobranie instancji i rozwiązanie problemu wybraną heurystyką
            BinpackData& problem = datasets[i];
            auto solution = heuristic.run(problem);
            problem.Solution = solution;
            // Obliczenie i wypisywanie Fill Factor
            double ff = solution.getObj();
            totalFF += ff;
            std::cout << "Instance " << problem.fileName
                      << ": Fill Factor = " << ff * 100.0 << "%" << std::endl;
            // Rysowanie rozwiązania do pliku
            if (draw_all_solutions) {
                drawToFile(problem, true, outputDir, ".png");
            }
        }
        if (!datasets.empty()) {
            std::cout << "Average Fill Factor: " << (totalFF / datasets.size()) * 100.0 << "%" << std::endl;
            std::cout << "Solutions saved to directory: " << outputDir << "/" << std::endl;
        }
    }

    void BinDrawer::populateColors() {
        Colors.clear();
        for( int r = 0; r < 4; r++ ) {
            for( int g = 0; g < 4; g++ ) {
                for( int b = 0; b < 4; b++ ) {
                    if (r == b && g == b && r == g) continue;
                    Colors.push_back( sf::Color( r*64, g*64, b*64 ) );
                }
            }
        }
        std::shuffle(Colors.begin(), Colors.end(), std::mt19937(1));
    }

void BinDrawer::print_specialist_results(
        std::vector<BinpackData>& datasets,
        const std::vector<std::vector<double>>& populationGenomes,
        BinpackConstructionHeuristic<nnutils::FFN>& heuristic,
        const std::string& outputDir, bool draw_all_best_solutions
    ) {
        // Przygotowanie pliku csv
        if (!std::filesystem::exists(outputDir)) {
            std::filesystem::create_directory(outputDir);
        }
        std::string csvPath = outputDir + "/evaluation_results.csv";
        std::ofstream csvFile(csvPath);
        if (!csvFile.is_open()) {
            std::cerr << "Error: Could not create CSV file at " << csvPath << std::endl;
            return;
        }

        std::cout << "Saving results to: " << csvPath << std::endl;

        // Nazwy w pierwszym wierszu pliku CSV
        csvFile << "Instance Name";
        for (size_t i = 0; i < populationGenomes.size(); ++i) {
            csvFile << ";Net_" << i;
        }
        csvFile << ";BEST_NET_ID;BEST_FF;WORST_NET_ID;WORST_FF;MEAN_FF;STDDEV_FF\n";

        // Wypisywanie dla kazdego zadania
        int taskCounter = 0;
        for (auto& problem : datasets) {
            taskCounter++;
            double bestFF = -DBL_MAX;
            int bestNetIdx = -1;
            double worstFF = DBL_MAX;
            int worstNetIdx = -1;
            double sumFF = 0.0;

            // ID zadania do csv
            csvFile << problem.fileName;

            // Ewaluacja wszystkich sieci dla biezacego zadania
            std::vector<double> currentScores(populationGenomes.size());
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < populationGenomes.size(); ++i) {
                auto localHeuristic = heuristic;
                localHeuristic.setParams(populationGenomes[i].data(), populationGenomes[i].size());

                auto sol = localHeuristic.run(problem);
                currentScores[i] = sol.getObj();
            }

            // Zapis wynikow dla kazdego zadania do pliku CSV
            for (int i = 0; i < populationGenomes.size(); ++i) {
                double ff = currentScores[i];
                csvFile << ";" << ff;

                if (ff > bestFF) {
                    bestFF = ff;
                    bestNetIdx = i;
                }
                if (ff < worstFF) {
                    worstFF = ff;
                    worstNetIdx = i;
                }
                sumFF += ff;
            }

            // Obliczanie średniej i odchylenia standardowego
            double meanFF = sumFF / currentScores.size();
            double stddevFF = 0.0;
            for (double ff : currentScores) {
                stddevFF += (ff - meanFF) * (ff - meanFF);
            }
            stddevFF = sqrt(stddevFF / currentScores.size());

            // Zapis podsumowania wiersza w CSV
            csvFile << ";" << bestNetIdx << ";" << bestFF << ";" << worstNetIdx << ";" << worstFF << ";" << meanFF << ";" << stddevFF << "\n";

            // Print wyników
            std::cout << "[" << taskCounter << "/" << datasets.size() << "] "
                      << std::left << std::setw(25) << problem.fileName
                      << " -> Best: Net_" << bestNetIdx
                      << " (FF: " << std::fixed << std::setprecision(2) << bestFF * 100.0 << "%)"
                      << ", Worst: Net_" << worstNetIdx
                      << " (FF: " << std::fixed << std::setprecision(2) << worstFF * 100.0 << "%)"
                      << ", Mean: " << meanFF * 100.0 << "%"
                      << ", Stddev: " << stddevFF * 100.0 << "%"
                      << std::endl;

            // Rysowanie najlepszego rozwiązania zadania
            if (draw_all_best_solutions && bestNetIdx >= 0) {
                heuristic.setParams(populationGenomes[bestNetIdx].data(), populationGenomes[bestNetIdx].size());
                problem.Solution = heuristic.run(problem);
                drawToFile(problem, true, outputDir, ".png");
            }
        }

        csvFile.close();
        std::cout << "\nEvaluation completed. \nCSV Table: " << csvPath << "\nImages: " << outputDir << "/" << std::endl;
    }

    void BinDrawer::drawToFile(const BinpackData &IOD, bool flip, const string dir,const string ext) {
        set<int> Bins;
        for (auto &BP : IOD.Solution.BPV) Bins.insert(BP.second.binIdx);

        for (auto binIdx : Bins) {
            // 1. Obliczanie faktycznych wymiarów zajętej przestrzeni
            long long maxHeuristicX = 0;
            long long maxHeuristicY = 0;

            for(auto &BP : IOD.Solution.BPV) {
                if (BP.second.binIdx != binIdx) continue;
                auto &B = IOD.BoxTypes[BP.first];
                auto Pos = BP.second;
                int itemLen = B.SizeX;
                int itemWid = B.SizeY;
                if (Pos.Rotated) swap(itemLen, itemWid);
                if (Pos.X + itemLen > maxHeuristicX) maxHeuristicX = Pos.X + itemLen;
                if (Pos.Y + itemWid > maxHeuristicY) maxHeuristicY = Pos.Y + itemWid;
            }

            // logicWidth to szerokość paska (stała dla zadania)
            // logicLength to wysokość ułożenia (zmienna dla zadania)
            float logicLength = (float)maxHeuristicX;
            float logicWidth  = (float)maxHeuristicY;
            if (logicLength <= 0) logicLength = 100.0f;
            if (logicWidth <= 0) logicWidth = 100.0f;

            // --- KLUCZOWA ZMIANA: STAŁA SZEROKOŚĆ W PIXELACH ---
            // Definiujemy, ile pikseli szerokości ma mieć sam "pasek" (bez marginesów)
            const float FIXED_STRIP_PX_WIDTH = 800.0f;

            // Skala zależy WYŁĄCZNIE od szerokości paska (logicWidth)
            float SCALE = FIXED_STRIP_PX_WIDTH / logicWidth;

            // Ograniczenie wysokości ze względu na limity techniczne tekstur (np. 16k px)
            if (SCALE * logicLength > 16000.0f) {
                SCALE = 16000.0f / logicLength;
            }

            float MARGIN = 3.0f;
            sf::Color marginColor(0,0,0);

            float CAN_W, CAN_H;
            if (flip) {
                // W wizualizacji pionowej (flip=true):
                // Szerokość obrazka (CAN_W) odpowiada szerokości paska (logicWidth)
                CAN_W = logicWidth * SCALE;  // To zawsze będzie ok. 800px
                CAN_H = logicLength * SCALE; // To będzie zmienne (wysokość wieży)
            } else {
                // W wizualizacji poziomej:
                // Szerokość obrazka (CAN_W) odpowiada długości paska
                CAN_W = logicLength * SCALE;
                CAN_H = logicWidth * SCALE;
            }

            unsigned int imgW = static_cast<unsigned int>(CAN_W + MARGIN * 2);
            unsigned int imgH = static_cast<unsigned int>(CAN_H + MARGIN);

            sf::RenderTexture window;
            if (!window.resize({imgW, imgH})) continue;

            window.clear(marginColor);

            // Białe wnętrze paska
            sf::RectangleShape binBackground({CAN_W, CAN_H});
            binBackground.setFillColor(sf::Color::White);
            binBackground.setPosition({MARGIN, 0.0f});
            window.draw(binBackground);

            // Tekst informacyjny
            sf::Text infoText(font);
            infoText.setString("FF: " + to_string_with_precision(IOD.getObj()*100, 2) + "%  H: " + std::to_string(maxHeuristicX));
            infoText.setCharacterSize(24);
            infoText.setFillColor(sf::Color::Black);
            infoText.setStyle(sf::Text::Bold);

            sf::FloatRect tb = infoText.getLocalBounds();
            infoText.setPosition({ (float)imgW - MARGIN - tb.size.x - 5.0f, 5.0f });

            // Rysowanie pudełek
            sf::Text txtNum(font);
            txtNum.setFillColor(sf::Color::Black);

            for( auto &BP: IOD.Solution.BPV ) {
                if (BP.second.binIdx != binIdx) continue;
                auto &B = IOD.BoxTypes[BP.first];
                auto Pos = BP.second;

                float hLen = B.SizeX * SCALE;
                float hWid = B.SizeY * SCALE;
                if (Pos.Rotated) swap(hLen, hWid);

                float hX = Pos.X * SCALE;
                float hY = Pos.Y * SCALE;

                float drawX, drawY, drawW, drawH;
                if (flip) {
                    drawX = MARGIN + hY;
                    drawY = CAN_H - hX - hLen;
                    drawW = hWid;
                    drawH = hLen;
                } else {
                    drawX = MARGIN + hX;
                    drawY = CAN_H - hY - hWid;
                    drawW = hLen;
                    drawH = hWid;
                }

                sf::RectangleShape Rect({drawW, drawH});
                Rect.setFillColor(B.idx < Colors.size() ? Colors[B.idx % Colors.size()] : sf::Color(128,128,128));
                Rect.setOutlineColor(sf::Color::Black);
                Rect.setOutlineThickness(1.0f);
                Rect.setPosition({drawX, drawY});
                window.draw(Rect);

                // Numerki (tekst centrowany w pudełku)
                txtNum.setString(std::to_string(B.idx));
                float minSide = std::min(drawW, drawH);
                txtNum.setCharacterSize(static_cast<unsigned int>(std::max(10.0f, minSide * 0.5f)));
                sf::FloatRect b = txtNum.getLocalBounds();
                txtNum.setOrigin({b.position.x + b.size.x / 2.0f, b.position.y + b.size.y / 2.0f});
                txtNum.setPosition(Rect.getPosition() + Rect.getSize() / 2.0f);

                if (b.size.x < drawW && b.size.y < drawH) window.draw(txtNum);
            }

            window.draw(infoText);
            window.display();

            string fn = dir + "/" + IOD.fileName + "_bin" + to_string(binIdx, 2) + ext;
            window.getTexture().copyToImage().saveToFile(fn);
        }
    }
}
