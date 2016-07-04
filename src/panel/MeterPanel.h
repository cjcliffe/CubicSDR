#pragma once

#include "GLPanel.h"

class MeterPanel : public GLPanel {
    
public:
    MeterPanel(std::string name, float low, float high, float current) {
        this->name = name;
        this->low = low;
        this->high = high;
        this->current = current;
        
        setBorderPx(1);
        setFill(GLPanel::GLPANEL_FILL_NONE);
        
        bgPanel.setCoordinateSystem(GLPanel::GLPANEL_Y_UP);
        bgPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_X);
        
        levelPanel.setBorderPx(0);
        levelPanel.setMarginPx(1);
        
        setPanelLevel(current, levelPanel);
        levelPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
        levelPanel.setBlend(GL_ONE, GL_ONE);

        bgPanel.addChild(&levelPanel);
        
        setPanelLevel(current, highlightPanel);
        highlightPanel.setBorderPx(0);
        highlightPanel.setMarginPx(1);
        highlightPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
        highlightPanel.setBlend(GL_ONE, GL_ONE);
        highlightPanel.visible = false;
        
        bgPanel.addChild(&highlightPanel);
        
        labelPanel.setSize(1.0, 0.1);
        labelPanel.setPosition(0.5, 1.0);
        labelPanel.setText(name,GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, true);
        labelPanel.setFill(GLPanel::GLPANEL_FILL_NONE);
        
        addChild(&labelPanel);
        
        valuePanel.setSize(1.0, 0.1);
        valuePanel.setPosition(0.5, -1.0);
        
        setValueLabel(std::to_string(int(current)));
        valuePanel.setFill(GLPanel::GLPANEL_FILL_NONE);
        
        addChild(&valuePanel);
    }
    
    void setName(std::string name_in) {
        name = name_in;
    }
    
    void setRange(float low, float high) {
        this->low = low;
        this->high = high;
    }
    
    void setValue(float value) {
        if (value > high) {
            value = high;
        }
        if (value < low) {
            value = low;
        }

        current = low + (value * (high-low));
        setValueLabel(std::to_string(int(current)));
        setPanelLevel(value, levelPanel);
    }
    
    void setHighlight(float value) {
        if (value > high) {
            value = high;
        }
        if (value < low) {
            value = low;
        }
        
        if (value == 0) {
            highlightPanel.visible = false;
        } else {
            setPanelLevel(value, highlightPanel);
            highlightPanel.visible = true;
        }
    }
    
    float getValue() {
        return current;
    }
    
    bool isMeterHit(CubicVR::vec2 mousePoint) {
        CubicVR::vec2 hitResult;
        
        if (bgPanel.hitTest(mousePoint, hitResult)) {
            return true;
        }
        
        return false;
    }

    float getMeterHitValue(CubicVR::vec2 mousePoint, GLPanel &panel) {
        CubicVR::vec2 hitResult;
        
        if (bgPanel.hitTest(mousePoint, hitResult)) {
            float hitLevel = hitResult.y;
        
            if (hitLevel < 0.0f) {
                hitLevel = 0.0f;
            }
            if (hitLevel > 1.0f) {
                hitLevel = 1.0f;
            }
            
            return low + (hitLevel * (high-low));
        } else {
            return 0;
        }
    }
    
protected:
    void drawPanelContents() {
        drawChildren();
    }

    void setValueLabel(std::string label) {
        valuePanel.setText(label,
                           GLFont::GLFONT_ALIGN_CENTER,
                           GLFont::GLFONT_ALIGN_CENTER,
                           true);

    }
    
    void setPanelLevel(float setValue, GLPanel &panel) {
         float valueNorm = (setValue - low) / (high - low);
         panel.setSize(1.0, valueNorm);
         panel.setPosition(0.0, (-1.0+(valueNorm)));
     }

private:
    std::string name;
    float low, high, current;
    GLPanel panel;
    GLPanel bgPanel;
    GLPanel levelPanel;
    GLPanel highlightPanel;
    GLTextPanel labelPanel;
    GLTextPanel valuePanel;
};