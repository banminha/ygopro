#include "buildin/common.h"

#include "buildin/rapidxml.hpp"
#include "buildin/rapidxml_print.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "buildin/stb_image_write.h"

#include "base/render_base.h"
#include "scene_mgr.h"

namespace ygopro
{

	CommonConfig commonCfg;
    CommonConfig stringCfg;
    
    void SceneMgr::Init(const std::wstring& layout) {
        start_time = std::chrono::system_clock::now().time_since_epoch().count();
        TextFile f(To<std::string>(layout));
        rapidxml::xml_document<> doc;
        doc.parse<0>(f.Data());
        rapidxml::xml_node<>* root = doc.first_node();
        rapidxml::xml_node<>* config_node = root->first_node();
        while(config_node) {
            std::string config_name = config_node->name();
            rapidxml::xml_attribute<>* attr = config_node->first_attribute();
            if(config_name == "integer") {
                std::string name = attr->value();
                int32_t val = To<int32_t>(attr->next_attribute()->value());
                int_config[name] = val;
            } else if(config_name == "float") {
                std::string name = attr->value();
                float val = To<float>(attr->next_attribute()->value());
                float_config[name] = val;
            } else if(config_name == "rect") {
                std::string name = attr->value();
                attr = attr->next_attribute();
                float x = To<float>(attr->value());
                attr = attr->next_attribute();
                float y = To<float>(attr->value());
                attr = attr->next_attribute();
                float w = To<float>(attr->value());
                attr = attr->next_attribute();
                float h = To<float>(attr->value());
                rect_config[name] = rectf{x, y, w, h};
            }
            config_node = config_node->next_sibling();
        }
    }
    
    void SceneMgr::Uninit() {
        if(current_scene != nullptr)
            current_scene.reset();
    }
    
    void SceneMgr::InitDraw() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
    }
    
    bool SceneMgr::Update() {
        uint64_t now_long = std::chrono::system_clock::now().time_since_epoch().count();
        now = (double)(now_long - start_time) * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
        if(current_scene != nullptr)
            return current_scene->Update();
        return true;
    }
    
    void SceneMgr::Draw() {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        if(current_scene != nullptr)
            current_scene->Draw();
    }
    
    void SceneMgr::SetFrameRate(double rate) {
        frame_interval = 1.0 / rate;
    }
    
    void SceneMgr::CheckFrameRate() {
        double now = GetGameTime();
        frame_check += frame_interval - (now - frame_time);
        if(frame_check >= 0.0)
            std::this_thread::sleep_for(std::chrono::microseconds((int32_t)(frame_interval * 1000000)));
        frame_time = now;
    }
    
    void SceneMgr::SetSceneSize(v2i sz) {
        scene_size = sz;
        if(current_scene != nullptr)
            current_scene->SetSceneSize(sz);
    }
    
    void SceneMgr::ScreenShot() {
        if(current_scene == nullptr)
            return;
        auto clip = current_scene->GetScreenshotClip();
        uint8_t* image_buff = new uint8_t[scene_size.x * scene_size.y * 4];
        uint8_t* clip_buff = new uint8_t[clip.width * clip.height * 4];
        glReadPixels(0, 0, scene_size.x, scene_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image_buff);
        for(int32_t h = 0; h < clip.height; ++h) {
            int32_t offset = scene_size.x * 4 * (scene_size.y - 1 - clip.top - h);
            memcpy(&clip_buff[clip.width * 4 * h], &image_buff[offset], clip.width * 4);
        }
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto tm = std::localtime(&t);
        char file[256];
        sprintf(file, "/%d%02d%02d-%ld.png", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, t);
        std::wstring wpath = commonCfg["screenshot_path"];
        std::string path = To<std::string>(wpath);
        path.append(file);
        stbi_write_png(path.c_str(), clip.width, clip.height, 4, clip_buff, 0);
        delete[] clip_buff;
        delete[] image_buff;
    }
}