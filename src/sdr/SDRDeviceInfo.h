#pragma once

#include <string>

class SDRDeviceInfo {
public:
    SDRDeviceInfo();
    
    std::string getDeviceId();
    
    bool isAvailable() const;
    void setAvailable(bool available);
    
    const std::string& getName() const;
    void setName(const std::string& name);
    
    const std::string& getSerial() const;
    void setSerial(const std::string& serial);
    
    const std::string& getTuner() const;
    void setTuner(const std::string& tuner);
    
    const std::string& getManufacturer() const;
    void setManufacturer(const std::string& manufacturer);
    
    const std::string& getProduct() const;
    void setProduct(const std::string& product);
    
private:
    std::string name;
    std::string serial;
    std::string product;
    std::string manufacturer;
    std::string tuner;
    bool available;
};
