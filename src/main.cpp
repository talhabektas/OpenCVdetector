/**
 * OMR System - Optical Mark Recognition
 * Main Application
 * 
 * Features:
 * - Real-time camera capture
 * - Perspective correction
 * - Multiple choice bubble detection
 * - Fill-in-the-blank handwriting recognition (OCR)
 * - True/False question support
 * - Automatic grading and scoring
 */

#include "CameraManager.h"
#include "PerspectiveCorrector.h"
#include "ImageEnhancer.h"
#include "BubbleDetector.h"
#include "HandwritingDetector.h"
#include "OCRProcessor.h"
#include "SheetStructureAnalyzer.h"
#include "AnswerKey.h"
#include "AnswerComparator.h"
#include "ScoreCalculator.h"
#include "ResultDisplayer.h"
#include "FileWriter.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <memory>
#include <chrono>

// Configuration
constexpr int CAMERA_ID = 0;
constexpr bool USE_CAMERA = true;  // Set to false to use test image
const std::string TEST_IMAGE_PATH = "test_exam.jpg";
const std::string ANSWER_KEY_PATH = "answer_key.txt";

/**
 * @brief Create example answer key
 */
void createExampleAnswerKey(AnswerKey& answerKey) {
    std::cout << "Örnek cevap anahtarı oluşturuluyor..." << std::endl;
    
    // Multiple choice questions (1-10)
    answerKey.addMultipleChoiceAnswer(1, 2);  // C (0-based: A=0, B=1, C=2, D=3, E=4)
    answerKey.addMultipleChoiceAnswer(2, 0);  // A
    answerKey.addMultipleChoiceAnswer(3, 3);  // D
    answerKey.addMultipleChoiceAnswer(4, 1);  // B
    answerKey.addMultipleChoiceAnswer(5, 4);  // E
    answerKey.addMultipleChoiceAnswer(6, 2);  // C
    answerKey.addMultipleChoiceAnswer(7, 0);  // A
    answerKey.addMultipleChoiceAnswer(8, 3);  // D
    answerKey.addMultipleChoiceAnswer(9, 1);  // B
    answerKey.addMultipleChoiceAnswer(10, 2); // C
    
    // Fill-in-the-blank questions (11-15)
    answerKey.addFillInBlankAnswer(11, "Istanbul");
    answerKey.addFillInBlankAnswer(12, "1923");
    answerKey.addFillInBlankAnswer(13, "Ankara");
    answerKey.addFillInBlankAnswer(14, "Mustafa Kemal");
    answerKey.addFillInBlankAnswer(15, "Cumhuriyet");
    
    // True/False questions (16-20)
    answerKey.addTrueFalseAnswer(16, true);
    answerKey.addTrueFalseAnswer(17, false);
    answerKey.addTrueFalseAnswer(18, true);
    answerKey.addTrueFalseAnswer(19, true);
    answerKey.addTrueFalseAnswer(20, false);
    
    std::cout << "Cevap anahtarı oluşturuldu: " << answerKey.getTotalQuestions() 
              << " soru" << std::endl;
}

/**
 * @brief Process exam sheet and extract answers
 */
std::vector<Answer> processExamSheet(
    const cv::Mat& examSheet,
    const std::vector<QuestionRegion>& regions,
    BubbleDetector& bubbleDetector,
    HandwritingDetector& handwritingDetector,
    OCRProcessor& ocrProcessor,
    ImageEnhancer& imageEnhancer) {
    
    std::vector<Answer> studentAnswers;
    
    for (const auto& region : regions) {
        Answer answer;
        answer.questionNumber = region.questionNumber;
        answer.type = static_cast<Answer::Type>(region.type);
        
        switch (region.type) {
            case QuestionRegion::MULTIPLE_CHOICE: {
                // Detect marked bubble
                int markedOption = bubbleDetector.detectMarkedAnswer(
                    examSheet,
                    region.region,
                    region.numOptions
                );
                answer.selectedOption = markedOption;
                
                if (markedOption >= 0) {
                    std::cout << "Soru " << region.questionNumber 
                             << ": Seçenek " << static_cast<char>('A' + markedOption) 
                             << std::endl;
                } else {
                    std::cout << "Soru " << region.questionNumber 
                             << ": İşaretlenmemiş veya çoklu işaret" << std::endl;
                }
                break;
            }
            
            case QuestionRegion::FILL_IN_BLANK: {
                // Check for handwriting
                if (handwritingDetector.hasHandwriting(examSheet, region.region)) {
                    // Extract and process with OCR
                    cv::Mat roi = handwritingDetector.extractHandwritingROI(
                        examSheet, region.region
                    );
                    
                    std::string text = ocrProcessor.recognizeText(roi);
                    answer.textAnswer = text;
                    
                    std::cout << "Soru " << region.questionNumber 
                             << ": \"" << text << "\"" << std::endl;
                } else {
                    answer.textAnswer = "";
                    std::cout << "Soru " << region.questionNumber 
                             << ": Boş" << std::endl;
                }
                break;
            }
            
            case QuestionRegion::TRUE_FALSE: {
                // Similar to multiple choice but with 2 options
                int markedOption = bubbleDetector.detectMarkedAnswer(
                    examSheet,
                    region.region,
                    2
                );
                answer.selectedOption = markedOption;
                
                if (markedOption >= 0) {
                    std::cout << "Soru " << region.questionNumber 
                             << ": " << (markedOption == 0 ? "Doğru" : "Yanlış") 
                             << std::endl;
                }
                break;
            }
        }
        
        studentAnswers.push_back(answer);
    }
    
    return studentAnswers;
}

/**
 * @brief Main application entry point
 */
int main(int argc, char** argv) {
    std::cout << "OMR Sistemi Başlatılıyor..." << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    try {
        // Initialize modules
        std::unique_ptr<CameraManager> camera;
        std::unique_ptr<PerspectiveCorrector> perspectiveCorrector = 
            std::make_unique<PerspectiveCorrector>();
        std::unique_ptr<ImageEnhancer> imageEnhancer = 
            std::make_unique<ImageEnhancer>();
        std::unique_ptr<BubbleDetector> bubbleDetector = 
            std::make_unique<BubbleDetector>(0.6);
        std::unique_ptr<HandwritingDetector> handwritingDetector = 
            std::make_unique<HandwritingDetector>(0.05);
        std::unique_ptr<OCRProcessor> ocrProcessor = 
            std::make_unique<OCRProcessor>("tur");
        std::unique_ptr<SheetStructureAnalyzer> sheetAnalyzer = 
            std::make_unique<SheetStructureAnalyzer>();
        std::unique_ptr<ResultDisplayer> resultDisplayer = 
            std::make_unique<ResultDisplayer>();
        std::unique_ptr<FileWriter> fileWriter = 
            std::make_unique<FileWriter>();
        
        // Check OCR initialization
        if (!ocrProcessor->isInitialized()) {
            std::cerr << "HATA: OCR başlatılamadı! Tesseract kurulu olduğundan emin olun." 
                     << std::endl;
            return -1;
        }
        
        // Create or load answer key
        AnswerKey answerKey;
        if (!answerKey.loadFromFile(ANSWER_KEY_PATH)) {
            std::cout << "Cevap anahtarı bulunamadı, örnek oluşturuluyor..." << std::endl;
            createExampleAnswerKey(answerKey);
            answerKey.saveToFile(ANSWER_KEY_PATH);
        }
        
        // Initialize grading
        AnswerComparator comparator(false); // Case-insensitive
        ScoreCalculator scoreCalculator(answerKey, comparator);
        scoreCalculator.setPartialCreditEnabled(true);
        scoreCalculator.setPartialCreditThreshold(0.7);
        
        cv::Mat examSheet;
        
        // Capture or load image
        if (USE_CAMERA && argc < 2) {
            camera = std::make_unique<CameraManager>();
            
            if (!camera->initCamera(CAMERA_ID)) {
                std::cerr << "HATA: Kamera başlatılamadı!" << std::endl;
                return -1;
            }
            
            std::cout << "\nKamera hazır. Sınav kağıdını görüntüye alın..." << std::endl;
            std::cout << "SPACE tuşu: Fotoğraf çek | ESC: Çıkış" << std::endl;
            
            // Live camera feed
            while (true) {
                cv::Mat frame = camera->captureFrame();
                
                // Show live feed
                cv::imshow("Kamera - SPACE ile çek, ESC ile çık", frame);
                
                int key = cv::waitKey(30);
                if (key == 27) { // ESC
                    std::cout << "Çıkış yapılıyor..." << std::endl;
                    return 0;
                } else if (key == 32) { // SPACE
                    examSheet = frame.clone();
                    std::cout << "Fotoğraf çekildi!" << std::endl;
                    break;
                }
            }
            
            cv::destroyAllWindows();
            
        } else {
            // Load from file
            std::string imagePath = (argc > 1) ? argv[1] : TEST_IMAGE_PATH;
            examSheet = cv::imread(imagePath);
            
            if (examSheet.empty()) {
                std::cerr << "HATA: Görüntü yüklenemedi: " << imagePath << std::endl;
                return -1;
            }
            
            std::cout << "Görüntü yüklendi: " << imagePath << std::endl;
        }
        
        // Start processing timer
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Step 1: Perspective correction
        std::cout << "\n1. Perspektif düzeltiliyor..." << std::endl;
        cv::Mat correctedSheet = perspectiveCorrector->correctPerspective(examSheet);
        
        // Step 2: Analyze sheet structure
        std::cout << "\n2. Sınav yapısı analiz ediliyor..." << std::endl;
        std::vector<QuestionRegion> regions = sheetAnalyzer->analyzeSheet(correctedSheet);
        
        // Visualize regions (optional)
        cv::Mat regionVis = sheetAnalyzer->visualizeRegions(correctedSheet, regions);
        cv::imshow("Tespit Edilen Bölgeler", regionVis);
        cv::waitKey(1000);
        
        // Step 3: Process answers
        std::cout << "\n3. Cevaplar işleniyor..." << std::endl;
        std::vector<Answer> studentAnswers = processExamSheet(
            correctedSheet,
            regions,
            *bubbleDetector,
            *handwritingDetector,
            *ocrProcessor,
            *imageEnhancer
        );
        
        // Step 4: Calculate score
        std::cout << "\n4. Puan hesaplanıyor..." << std::endl;
        ExamScore score = scoreCalculator.calculateScore(studentAnswers);
        
        // Calculate processing time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        ).count();
        
        std::cout << "\nİşleme süresi: " << duration << " ms" << std::endl;
        
        // Step 5: Display results
        std::cout << "\n5. Sonuçlar gösteriliyor..." << std::endl;
        resultDisplayer->displayScoreSummary(score);
        resultDisplayer->displayDetailedResults(score);
        
        auto stats = scoreCalculator.getStatistics(score);
        resultDisplayer->displayStatistics(stats);
        
        // Create visual results
        cv::Mat resultImage = resultDisplayer->createVisualResults(
            correctedSheet,
            score,
            regions
        );
        
        resultDisplayer->displayInWindow("Sonuçlar", resultImage, 0);
        
        // Step 6: Save results
        std::cout << "\n6. Sonuçlar kaydediliyor..." << std::endl;
        
        std::string timestamp = fileWriter->createTimestampedFilename("exam", "");
        timestamp = timestamp.substr(0, timestamp.length() - 0); // Remove extension
        
        fileWriter->saveResultsToText(timestamp + "_results.txt", score);
        fileWriter->saveResultsToCSV(timestamp + "_results.csv", score);
        fileWriter->saveResultImage(timestamp + "_results.jpg", resultImage);
        
        std::cout << "\n✓ İşlem tamamlandı!" << std::endl;
        std::cout << "Sonuçlar '" << timestamp << "' öneki ile kaydedildi." << std::endl;
        
        // Wait for user to close
        std::cout << "\nÇıkmak için herhangi bir tuşa basın..." << std::endl;
        cv::waitKey(0);
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\nFATAL HATA: " << e.what() << std::endl;
        return -1;
    }
}
