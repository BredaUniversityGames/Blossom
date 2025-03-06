#include <precompiled/editor_precompiled.hpp>
#include <noise_editor/noise_editor.hpp>

#include <random>
#include <sstream>
#include <array>
#include "core/engine.hpp"
#include "resources/resource_manager.hpp"
#include "resources/image/image_gl.hpp"
#include "tools/log.hpp"

using namespace bee;

static bool MatchingGroup(const std::vector<const char*>& a, const std::vector<const char*>& b)
{
    std::string aString;
    for (const char* c : a)
    {
        aString.append(c);
        aString.push_back('\t');
    }

    std::string bString;
    for (const char* c : b)
    {
        bString.append(c);
        bString.push_back('\t');
    }

    return aString == bString;
}

template <typename T>
static bool MatchingMembers(const std::vector<T>& a, const std::vector<T>& b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (size_t i = 0; i < a.size(); i++)
    {
        if (strcmp(a[i].name, b[i].name) != 0)
        {
            return false;
        }
    }
    return true;
}

static std::string TimeWithUnits(int64_t time, int significantDigits = 3)
{
    if (time == 0)
    {
        return "0us";
    }

    double f = time / 1e+3;

    int d = (int)::ceil(::log10(::abs(f))); /*digits before decimal point*/
    std::stringstream ss;
    ss.precision(std::max(significantDigits - d, 0));
    ss << std::fixed << f << "us";
    return ss.str();
}

template <typename... Args>
std::string string_format(const char* format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format, args...);
    if (size_s <= 0)
    {
        return "";
    }
    auto size = static_cast<size_t>(size_s);
    std::string buf(size, 0);
    std::snprintf(buf.data(), size, format, args...);
    return buf;
}

template <typename... T>
static bool DoHoverPopup(const char* format, T... args)
{
    if (ImGui::IsItemHovered())
    {
        std::string hoverTxt = string_format(format, args...);

        if (hoverTxt.empty())
        {
            return false;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(hoverTxt.c_str());
        ImGui::EndTooltip();
        ImGui::PopStyleVar();
        return true;
    }
    return false;
}

NoiseEditor::Node::Node(NoiseEditor& e, FastNoise::NodeData* nodeData, bool generatePreview, int id)
    : Node(e, std::unique_ptr<FastNoise::NodeData>(nodeData), generatePreview, id)
{
}

NoiseEditor::Node::Node(NoiseEditor& e, std::unique_ptr<FastNoise::NodeData>&& nodeData, bool generatePreview,
    int id)
    : editor(e), data(std::move(nodeData)), nodeId(id ? id : e.GetFreeNodeId())
{
    assert(!e.FindNodeFromId(id));

    if (generatePreview)
    {
        GeneratePreview();
    }
}

void NoiseEditor::Node::GeneratePreview(bool nodeTreeChanged, bool benchmark)
{
    static std::array<float, NoiseSize* NoiseSize> noiseData;

    serialised = FastNoise::Metadata::SerialiseNodeData(data.get(), true);
    auto generator = FastNoise::NewFromEncodedNodeTree(serialised.c_str(), editor.mMaxSIMDLevel);

    if (!benchmark && nodeTreeChanged)
    {
        generateAverages.clear();
    }

    if (generator)
    {
        auto genRGB = FastNoise::New<FastNoise::ConvertRGBA8>(editor.mMaxSIMDLevel);
        genRGB->SetSource(generator);

        auto startTime = std::chrono::high_resolution_clock::now();

        editor.GenerateNodePreviewNoise(genRGB.get(), noiseData.data(), mNodeFrequency, mNodeSeed);

        generateAverages.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - startTime)
            .count() -
            editor.mOverheadNode.totalGenerateNs);

        std::sort(generateAverages.begin(), generateAverages.end());

        totalGenerateNs = generateAverages[generateAverages.size() / 3];
    }
    else
    {
        std::fill(noiseData.begin(), noiseData.end(), 0.0f);
        serialised.clear();
        totalGenerateNs = 0;
    }

    if (benchmark)
    {
        return;
    }

    noiseTexture = Engine.Resources().Images().FromRawData(noiseData.data(), ImageFormat::RGBA8, NoiseSize, NoiseSize);
    preview++;

    for (auto& node : editor.mNodes)
    {
        for (FastNoise::NodeData* link : node.second.GetNodeIDLinks())
        {
            if (link == data.get())
            {
                node.second.GeneratePreview(nodeTreeChanged);
            }
        }
    }

    if (nodeTreeChanged)
    {
        if (editor.mSelectedNode == data.get())
        {
            editor.ChangeSelectedNode(data.get());
        }

        // Save nodes to ini
        if (ImGui::GetCurrentContext() && ImGui::GetCurrentContext()->SettingsDirtyTimer <= 0.0f)
        {
            ImGui::GetCurrentContext()->SettingsDirtyTimer = ImGui::GetIO().IniSavingRate;
        }
    }
}

std::vector<FastNoise::NodeData*> NoiseEditor::Node::GetNodeIDLinks()
{
    std::vector<FastNoise::NodeData*> links;
    links.reserve(data->nodeLookups.size() + data->hybrids.size());

    for (FastNoise::NodeData* link : data->nodeLookups)
    {
        links.emplace_back(link);
    }

    for (auto& link : data->hybrids)
    {
        links.emplace_back(link.first);
    }

    assert(links.size() < 16);

    return links;
}

int64_t NoiseEditor::Node::GetLocalGenerateNs()
{
    int64_t localTotal = totalGenerateNs;

    for (FastNoise::NodeData* link : GetNodeIDLinks())
    {
        auto find = editor.mNodes.find(link);

        if (find != editor.mNodes.end())
        {
            localTotal -= find->second.totalGenerateNs;
        }
    }

    return std::max<int64_t>(localTotal, 0);
}

FastNoise::NodeData*& NoiseEditor::Node::GetNodeLink(int attributeId)
{
    attributeId &= 15;

    if (attributeId < (int)data->nodeLookups.size())
    {
        return data->nodeLookups[attributeId];
    }
    else
    {
        attributeId -= (int)data->nodeLookups.size();
        return data->hybrids[attributeId].first;
    }
}

void NoiseEditor::Node::AutoPositionChildNodes(ImVec2 nodePos, float verticalSpacing)
{
    auto nodeLinks = GetNodeIDLinks();
    nodeLinks.erase(std::remove(nodeLinks.begin(), nodeLinks.end(), nullptr), nodeLinks.end());

    if (nodeLinks.empty())
    {
        return;
    }

    ImVec2 nodeSpacing = { 280, verticalSpacing };

    nodePos.x -= nodeSpacing.x;
    nodePos.y -= nodeSpacing.y * 0.5f * (nodeLinks.size() - 1);

    for (FastNoise::NodeData* link : nodeLinks)
    {
        ImNodes::SetNodeScreenSpacePos(editor.mNodes.at(link).nodeId, nodePos);

        editor.mNodes.at(link).AutoPositionChildNodes(nodePos, nodeLinks.size() > 1 ? verticalSpacing * 0.6f : verticalSpacing);
        nodePos.y += nodeSpacing.y;
    }
}

bool NoiseEditor::MetadataMenuItem::CanDraw(std::function<bool(const FastNoise::Metadata*)> isValid) const
{
    return !isValid || isValid(metadata);
}

const FastNoise::Metadata* NoiseEditor::MetadataMenuItem::DrawUI(
    std::function<bool(const FastNoise::Metadata*)> isValid, bool drawGroups) const
{
    std::string format = FastNoise::Metadata::FormatMetadataNodeName(metadata, true);

    if (ImGui::MenuItem(format.c_str()))
    {
        return metadata;
    }
    return nullptr;
}

bool NoiseEditor::MetadataMenuGroup::CanDraw(std::function<bool(const FastNoise::Metadata*)> isValid) const
{
    for (const auto& item : items)
    {
        if (item->CanDraw(isValid))
        {
            return true;
        }
    }
    return false;
}

const FastNoise::Metadata* NoiseEditor::MetadataMenuGroup::DrawUI(
    std::function<bool(const FastNoise::Metadata*)> isValid, bool drawGroups) const
{
    const FastNoise::Metadata* returnPressed = nullptr;

    bool doGroup = drawGroups && name[0] != 0;

    if (!doGroup || ImGui::BeginMenu(name))
    {
        for (const auto& item : items)
        {
            if (item->CanDraw(isValid))
            {
                if (auto pressed = item->DrawUI(isValid, drawGroups))
                {
                    returnPressed = pressed;
                }
            }
        }
        if (doGroup)
        {
            ImGui::EndMenu();
        }
    }
    return returnPressed;
}

void NoiseEditor::Node::SerialiseIncludingDependancies(ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer,
    std::unordered_set<int>& serialisedNodeIds)
{
    if (serialisedNodeIds.find(nodeId) != serialisedNodeIds.end())
    {
        return;
    }

    for (FastNoise::NodeData* nodeData : GetNodeIDLinks())
    {
        if (nodeData)
        {
            editor.mNodes.at(nodeData).SerialiseIncludingDependancies(handler, buffer, serialisedNodeIds);
        }
    }

    for (const auto& var : data->variables)
    {
        buffer->appendf("variable=%d\n", var.i);
    }
    for (const auto& node : data->nodeLookups)
    {
        buffer->appendf("node=%d\n", node ? editor.mNodes.at(node).nodeId : 0);
    }
    for (const auto& hybrid : data->hybrids)
    {
        buffer->appendf("hybrid=%i:%f\n", hybrid.first ? editor.mNodes.at(hybrid.first).nodeId : 0, hybrid.second);
    }

    // id must be after setting all members, it verifies and creates the node
    buffer->appendf("id=%i\n", nodeId);

    // Must be after node creation
    ImVec2 gridPos = ImNodes::GetNodeGridSpacePos(nodeId);
    buffer->appendf("grid_pos=%f:%f\n", gridPos.x, gridPos.y);

    serialisedNodeIds.emplace(nodeId);
}

void NoiseEditor::SetupSettingsHandlers()
{
    ImGuiSettingsHandler nodeSettings;
    nodeSettings.TypeName = "NoiseToolNodeData";
    nodeSettings.TypeHash = ImHashStr(nodeSettings.TypeName);
    nodeSettings.UserData = this;
    nodeSettings.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* outBuf)
        {
            auto* nodeEditor = (NoiseEditor*)handler->UserData;

            std::unordered_set<int> serialisedNodeIds;

            // Save all root nodes
            for (auto& node : nodeEditor->mNodes)
            {
                bool hasReference = false;

                for (auto& linkNode : nodeEditor->mNodes)
                {
                    auto links = linkNode.second.GetNodeIDLinks();

                    if (std::find(links.begin(), links.end(), node.first) != links.end())
                    {
                        hasReference = true;
                        break;
                    }
                }

                if (!hasReference)
                {
                    node.second.SerialiseIncludingDependancies(handler, outBuf, serialisedNodeIds);
                }
            }
        };
    nodeSettings.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void*
        {
            //int metadataId;
            /*if (sscanf(name, "Node:%d", &metadataId) == 1)
            {
                if (const FastNoise::Metadata* metadata = FastNoise::Metadata::GetFromId(metadataId))
                {
                    FastNoise::NodeData* nodeData = new FastNoise::NodeData(metadata);
                    nodeData->nodeLookups.clear();
                    nodeData->variables.clear();
                    nodeData->hybrids.clear();
                    return nodeData;
                }
            }*/

            return nullptr;
        };
    nodeSettings.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
        {
            auto* nodeEditor = (NoiseEditor*)handler->UserData;
            auto* nodeData = (FastNoise::NodeData*)entry;

        //ImVec2 imVec2;
        //float f;
        //int i;
        //if (sscanf(line, "grid_pos=%f:%f", &imVec2.x, &imVec2.y) == 2)
        //{
        //    auto find = nodeEditor->mNodes.find(nodeData);
        //    if (find != nodeEditor->mNodes.end())
        //    {
        //        ImNodes::SetNodeGridSpacePos(find->second.nodeId, imVec2);
        //    }
        //}
        //else if (sscanf(line, "variable=%d", &i) == 1)
        //{
        //    nodeData->variables.push_back(i);
        //}
        //else if (sscanf(line, "node=%d", &i) == 1)
        //{
        //    Node* link = nodeEditor->FindNodeFromId(i);

            //    nodeData->nodeLookups.push_back(link ? link->data.get() : nullptr);
            //}
            //else if (sscanf(line, "hybrid=%d:%f", &i, &f) == 2)
            //{
            //    Node* link = nodeEditor->FindNodeFromId(i);

            //    nodeData->hybrids.emplace_back(link ? link->data.get() : nullptr, f);
            //}
            //else if (sscanf(line, "id=%d", &i) == 1)
            //{
            //    // Check the data is valid (node class may have changed)
            //    if (nodeData->variables.size() == nodeData->metadata->memberVariables.size() &&
            //        nodeData->nodeLookups.size() == nodeData->metadata->memberNodeLookups.size() &&
            //        nodeData->hybrids.size() == nodeData->metadata->memberHybrids.size())
            //    {
            //        if (!nodeEditor->FindNodeFromId(i) &&
            //            nodeEditor->mNodes.try_emplace(nodeData, *nodeEditor, nodeData, true, i).second)
            //        {
            //            return;
            //        }
            //    }

            //    delete nodeData;
            //}
        };

    ImGuiSettingsHandler editorSettings;
    editorSettings.TypeName = "NoiseToolNodeGraph";
    editorSettings.TypeHash = ImHashStr(editorSettings.TypeName);
    editorSettings.UserData = this;
    editorSettings.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* outBuf)
        {
            auto* nodeEditor = (NoiseEditor*)handler->UserData;
            outBuf->appendf("\n[%s][Settings]\n", handler->TypeName);

            ImVec2 gridOffset = ImNodes::EditorContextGetPanning();
            outBuf->appendf("grid_offset=%f:%f\n", gridOffset.x, gridOffset.y);

            // outBuf->appendf("frequency=%f\n", nodeEditor->mNodeFrequency);
            // outBuf->appendf("seed=%d\n", nodeEditor->mNodeSeed);
            // outBuf->appendf("gen_type=%d\n", (int)nodeEditor->mNodeGenType);
        };
    editorSettings.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void*
        {
            if (strcmp(name, "Settings") == 0)
            {
                return handler->UserData;
            }

            return nullptr;
        };
    editorSettings.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
        {
            auto* nodeEditor = (NoiseEditor*)handler->UserData;

            /*ImVec2 imVec2;
            if (sscanf(line, "grid_offset=%f:%f", &imVec2.x, &imVec2.y) == 2)
            {
                ImNodes::EditorContextResetPanning(imVec2);
            }

            sscanf(line, "frequency=%f", &nodeEditor->mNodeFrequency);
            sscanf(line, "seed=%d", &nodeEditor->mNodeSeed);*/
            // sscanf(line, "gen_type=%d", (int*)&nodeEditor->mNodeGenType);
        };

    // ImGuiExtra::AddOrReplaceSettingsHandler(editorSettings);
    // ImGuiExtra::AddOrReplaceSettingsHandler(nodeSettings);
}

NoiseEditor::NoiseEditor()
    : mOverheadNode(*this, new FastNoise::NodeData(&FastNoise::Metadata::Get<FastNoise::Constant>()), false)
{
//#ifndef NDEBUG
//    mNodeBenchmarkMax = 1;
//#endif

    srand(
        static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count())
    );

    SetupSettingsHandlers();

    // Create Metadata context menu tree
    std::unordered_map<std::string, MetadataMenuGroup*> groupMap;
    auto* root = new MetadataMenuGroup("");
    mContextMetadata.emplace_back(root);

    auto menuSort = [](const MetadataMenu* a, const MetadataMenu* b) { return std::strcmp(a->GetName(), b->GetName()) < 0; };

    for (const FastNoise::Metadata* metadata : FastNoise::Metadata::GetAll())
    {
        auto* metaDataGroup = root;

        std::string groupTree;

        for (const char* group : metadata->groups)
        {
            groupTree += group;
            auto find = groupMap.find(groupTree);
            if (find == groupMap.end())
            {
                auto* newGroup = new MetadataMenuGroup(group);
                mContextMetadata.emplace_back(newGroup);
                metaDataGroup->items.emplace_back(newGroup);
                find = groupMap.emplace(groupTree, newGroup).first;

                std::sort(metaDataGroup->items.begin(), metaDataGroup->items.end(), menuSort);
            }

            metaDataGroup = find->second;
            groupTree += '\t';
        }

        metaDataGroup->items.emplace_back(mContextMetadata.emplace_back(new MetadataMenuItem(metadata)).get());
        std::sort(metaDataGroup->items.begin(), metaDataGroup->items.end(), menuSort);
    }
}

void NoiseEditor::DoNodeBenchmarks()
{
    // Benchmark overhead every frame to keep it accurate
    if (mOverheadNode.generateAverages.size() >= 512)
    {
        std::default_random_engine engine((uint32_t)mOverheadNode.totalGenerateNs);
        std::uniform_int_distribution<size_t> randomInt{ 0ul, mOverheadNode.generateAverages.size() - 1 };

        mOverheadNode.generateAverages.erase(mOverheadNode.generateAverages.begin() + randomInt(engine));
    }

    mOverheadNode.GeneratePreview(false, true);

    // 1 node benchmark per frame
    if (mNodeBenchmarkIndex >= (int32_t)mNodes.size())
    {
        mNodeBenchmarkIndex = 0;
    }

    for (auto itr = std::next(mNodes.begin(), mNodeBenchmarkIndex); itr != mNodes.end(); ++itr)
    {
        mNodeBenchmarkIndex++;
        if (!itr->second.serialised.empty() && itr->second.generateAverages.size() < mNodeBenchmarkMax)
        {
            itr->second.GeneratePreview(false, true);
            break;
        }
    }
}

void NoiseEditor::Show(BlossomGame& game)
{
    //const ImGuiViewport* viewport = ImGui::GetMainViewport();
    //ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::SetNextWindowSize(ImVec2(963, 634), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(8, 439), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Noise Editor"))
    {
        UpdateSelected();

        bool edited = false;
        ImGui::PushItemWidth(82.0f);

        auto currentNode = mNodes.find(mSelectedNode);
        if (currentNode != mNodes.end())
        {
            float oldFreq = currentNode->second.mNodeFrequency;
            edited |= (oldFreq != (currentNode->second.mNodeFrequency = mNoiseTexture.GetFrequency()));
            int oldSeed = currentNode->second.mNodeSeed;
            edited |= (oldSeed != (currentNode->second.mNodeSeed = mNoiseTexture.GetSeed()));
        }

        if (ImGui::Button("Retest Node Performance"))
        {
            for (auto& node : mNodes)
            {
                node.second.generateAverages.clear();
            }
        }
        ImGui::SameLine();

        std::string simdTxt = "Current SIMD Level: ";
        simdTxt += GetSIMDLevelName(mActualSIMDLevel);
        ImGui::TextUnformatted(simdTxt.c_str());

        ImGui::PopItemWidth();

        CheckLinks();

        if (edited)
        {
            if (currentNode != mNodes.end())
            {
                currentNode->second.GeneratePreview(false);
            }

            if (ImGui::GetCurrentContext() && ImGui::GetCurrentContext()->SettingsDirtyTimer <= 0.0f)
            {
                ImGui::GetCurrentContext()->SettingsDirtyTimer = ImGui::GetIO().IniSavingRate;
            }
        }

        ImNodes::BeginNodeEditor();

        DoHelp();

        DoContextMenu();

        DoNodes();

        ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomLeft);

#if 0   // no implementation for it as of yet
        if (ImGui::IsWindowHovered())
        {
            auto zoom = ImNodes::EditorContextGetZoom() + ImGui::GetIO().MouseWheel * 0.1f;
            ImNodes::EditorContextSetZoom(zoom, ImGui::GetMousePos());
        }
#endif

        ImNodes::EndNodeEditor();

       
    }
    ImGui::End();

    DoNodeBenchmarks();

    mNoiseTexture.Draw();
}

void NoiseEditor::CheckLinks()
{
    // Check for new links
    int startNodeId, endNodeId;
    int startAttr, endAttr;
    bool createdFromSnap;
    if (ImNodes::IsLinkCreated(&startNodeId, &startAttr, &endNodeId, &endAttr, &createdFromSnap))
    {
        Node* startNode = FindNodeFromId(startNodeId);
        Node* endNode = FindNodeFromId(endNodeId);

        if (startNode && endNode)
        {
            auto& link = endNode->GetNodeLink(endAttr);

            if (!createdFromSnap || !link)
            {
                link = startNode->data.get();
                endNode->GeneratePreview();
            }
        }
    }

    int attrStart;
    if (ImNodes::IsLinkDropped(&attrStart, false))
    {
        Node* node = FindNodeFromId(Node::GetNodeIdFromAttribute(attrStart));

        if (node && (attrStart & Node::AttributeBitMask) == Node::AttributeBitMask)
        {
            mDroppedLinkNode = node->data.get();
            mDroppedLink = true;
        }
    }
}

void NoiseEditor::DeleteNode(FastNoise::NodeData* nodeData)
{
    mNodes.erase(nodeData);

    for (auto& node : mNodes)
    {
        bool changed = false;
        int attrId = node.second.GetStartingAttributeId();

        for (FastNoise::NodeData* link : node.second.GetNodeIDLinks())
        {
            if (link == nodeData)
            {
                node.second.GetNodeLink(attrId) = nullptr;
                changed = true;
            }
            attrId++;
        }

        if (changed)
        {
            node.second.GeneratePreview();
        }
    }
}

void NoiseEditor::UpdateSelected()
{
    std::vector<int> linksToDelete;
    int selectedLinkCount = ImNodes::NumSelectedLinks();

    bool delKeyPressed = ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete), false) ||
        ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace), false);

    if (selectedLinkCount && delKeyPressed)
    {
        linksToDelete.resize(selectedLinkCount);
        ImNodes::GetSelectedLinks(linksToDelete.data());
    }

    int destroyedLinkId;
    if (ImNodes::IsLinkDestroyed(&destroyedLinkId))
    {
        linksToDelete.push_back(destroyedLinkId);
    }

    for (int deleteID : linksToDelete)
    {
        for (auto& node : mNodes)
        {
            bool changed = false;
            int attributeId = node.second.GetStartingAttributeId();

            for (FastNoise::NodeData* link : node.second.GetNodeIDLinks())
            {
                (void)link;
                if (attributeId == deleteID)
                {
                    node.second.GetNodeLink(attributeId) = nullptr;
                    changed = true;
                }
                attributeId++;
            }

            if (changed)
            {
                node.second.GeneratePreview();
            }
        }
    }

    int selectedNodeCount = ImNodes::NumSelectedNodes();

    if (selectedNodeCount && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete), false))
    {
        std::vector<int> selected(selectedNodeCount);

        ImNodes::GetSelectedNodes(selected.data());

        for (int deleteID : selected)
        {
            if (Node* node = FindNodeFromId(deleteID))
            {
                DeleteNode(node->data.get());
            }
        }
    }
}

void NoiseEditor::SetSIMDLevel(FastSIMD::eLevel lvl)
{
    mMaxSIMDLevel = lvl;

    mOverheadNode.generateAverages.clear();
    DoNodeBenchmarks();

    for (auto& node : mNodes)
    {
        node.second.generateAverages.clear();
        node.second.GeneratePreview(false);
    }

    ChangeSelectedNode(mSelectedNode);
}

void NoiseEditor::DoNodes()
{
    for (auto& node : mNodes)
    {
        ImNodes::BeginNode(node.second.nodeId);

        ImNodes::BeginNodeTitleBar();
        std::string formatName = FastNoise::Metadata::FormatMetadataNodeName(node.second.data->metadata);
        ImGui::TextUnformatted(formatName.c_str());

#ifdef NDEBUG
        DoHoverPopup("%s", node.first->metadata->description);
#else
        DoHoverPopup("%s : %d\n%s", node.first->metadata->name, node.first->metadata->id, node.first->metadata->description);
#endif
        std::string performanceString = TimeWithUnits(node.second.GetLocalGenerateNs());

        ImGui::SameLine(Node::NoiseSize - ImGui::CalcTextSize(performanceString.c_str()).x);
        ImGui::TextUnformatted(performanceString.c_str());

        if (ImGui::IsItemHovered())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
            ImGui::SetTooltip("Total: %s", TimeWithUnits(node.second.totalGenerateNs).c_str());
            ImGui::PopStyleVar();
        }

        ImNodes::EndNodeTitleBar();

        // Right click node title to change node type
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::MenuItem("Convert To:", nullptr, nullptr, false);

            auto& nodeMetadata = node.second.data->metadata;
            auto newMetadata = mContextMetadata.front()->DrawUI(
                [nodeMetadata](const FastNoise::Metadata* metadata)
                { return metadata != nodeMetadata && MatchingGroup(metadata->groups, nodeMetadata->groups); },
                false);

            if (newMetadata)
            {
                if (MatchingMembers(newMetadata->memberVariables, nodeMetadata->memberVariables) &&
                    MatchingMembers(newMetadata->memberNodeLookups, nodeMetadata->memberNodeLookups) &&
                    MatchingMembers(newMetadata->memberHybrids, nodeMetadata->memberHybrids))
                {
                    nodeMetadata = newMetadata;
                }
                else
                {
                    FastNoise::NodeData newData(newMetadata);

                    std::queue<FastNoise::NodeData*> links;

                    for (FastNoise::NodeData* link : node.second.data->nodeLookups)
                    {
                        links.emplace(link);
                    }
                    for (auto& link : node.second.data->hybrids)
                    {
                        links.emplace(link.first);
                    }

                    for (auto& link : newData.nodeLookups)
                    {
                        if (links.empty()) break;
                        link = links.front();
                        links.pop();
                    }
                    for (auto& link : newData.hybrids)
                    {
                        if (links.empty()) break;
                        link.first = links.front();
                        links.pop();
                    }

                    *node.second.data = std::move(newData);
                }

                node.second.GeneratePreview();
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        ImGui::PushItemWidth(60.0f);

        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkCreationOnSnap);
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
        int attributeId = node.second.GetStartingAttributeId();
        auto& nodeMetadata = node.second.data->metadata;
        auto& nodeData = node.second.data;

        for (auto& memberNode : nodeMetadata->memberNodeLookups)
        {
            ImNodes::BeginInputAttribute(attributeId++);
            formatName = FastNoise::Metadata::FormatMetadataMemberName(memberNode);
            ImGui::TextUnformatted(formatName.c_str());
            ImNodes::EndInputAttribute();

            DoHoverPopup(memberNode.description);
        }

        for (size_t i = 0; i < node.second.data->metadata->memberHybrids.size(); i++)
        {
            ImNodes::BeginInputAttribute(attributeId++);

            bool isLinked = node.second.data->hybrids[i].first;
            const char* floatFormat = "%.3f";

            if (isLinked)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                floatFormat = "";
            }

            formatName = FastNoise::Metadata::FormatMetadataMemberName(nodeMetadata->memberHybrids[i]);

            if (ImGui::DragFloat(formatName.c_str(), &nodeData->hybrids[i].second, 0.02f, 0, 0, floatFormat))
            {
                node.second.GeneratePreview();
            }

            if (isLinked)
            {
                ImGui::PopItemFlag();
            }
            ImNodes::EndInputAttribute();
            DoHoverPopup(nodeMetadata->memberHybrids[i].description);
        }

        for (size_t i = 0; i < nodeMetadata->memberVariables.size(); i++)
        {
            ImNodes::BeginStaticAttribute(0);

            auto& nodeVar = nodeMetadata->memberVariables[i];

            formatName = FastNoise::Metadata::FormatMetadataMemberName(nodeVar);

            switch (nodeVar.type)
            {
            case FastNoise::Metadata::MemberVariable::EFloat:
            {
                if (ImGui::DragFloat(formatName.c_str(), &nodeData->variables[i].f, 0.02f, nodeVar.valueMin.f,
                    nodeVar.valueMax.f))
                {
                    node.second.GeneratePreview();
                }
            }
            break;
            case FastNoise::Metadata::MemberVariable::EInt:
            {
                if (ImGui::DragInt(formatName.c_str(), &nodeData->variables[i].i, 0.2f, nodeVar.valueMin.i,
                    nodeVar.valueMax.i))
                {
                    node.second.GeneratePreview();
                }
            }
            break;
            case FastNoise::Metadata::MemberVariable::EEnum:
            {
                if (ImGui::Combo(formatName.c_str(), &nodeData->variables[i].i, nodeVar.enumNames.data(),
                    (int)nodeVar.enumNames.size()))
                {
                    node.second.GeneratePreview();
                }
            }
            break;
            }

            ImNodes::EndStaticAttribute();
            DoHoverPopup(nodeMetadata->memberVariables[i].description);
        }

        ImGui::PopItemWidth();
        ImNodes::PopAttributeFlag();
        ImNodes::BeginOutputAttribute(node.second.GetOutputAttributeId(), ImNodesPinShape_QuadFilled);

        glm::vec2 noiseSize = { (float)Node::NoiseSize, (float)Node::NoiseSize };
        if (mSelectedNode == node.first && !node.second.serialised.empty())
        {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImGui::RenderFrame(cursorPos - ImVec2(1, 1), cursorPos + ImVec2(noiseSize.x, noiseSize.y) + ImVec2(1, 1), IM_COL32(255, 0, 0, 200),
                false);
        }

        if (auto noiseImage = node.second.noiseTexture.Retrieve()) {
            const auto lm = static_cast<size_t>(noiseImage->handle);
            const ImTextureID id = reinterpret_cast<void*>(lm);
            ImGui::Image(id, ImVec2(noiseSize.x, noiseSize.y));
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            ChangeSelectedNode(node.first);
        }
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();
    }

    // Do current node links
    for (auto& node : mNodes)
    {
        int attributeId = node.second.GetStartingAttributeId();

        for (FastNoise::NodeData* link : node.second.GetNodeIDLinks())
        {
            if (link)
            {
                ImNodes::Link(attributeId, mNodes.at(link).GetOutputAttributeId(), attributeId);
            }
            attributeId++;
        }
    }
}

NoiseEditor::Node& NoiseEditor::AddNode(ImVec2 startPos, const FastNoise::Metadata* metadata,
    bool generatePreview)
{
    FastNoise::NodeData* nodeData = new FastNoise::NodeData(metadata);

    auto newNode = mNodes.try_emplace(nodeData, *this, nodeData, generatePreview);

    ImNodes::SetNodeScreenSpacePos(newNode.first->second.nodeId, startPos);

    if (mNodes.size() == 1)
    {
        ChangeSelectedNode(nodeData);
    }

    return newNode.first->second;
}

bool NoiseEditor::AddNodeFromEncodedString(const char* string, ImVec2 nodePos)
{
    std::vector<std::unique_ptr<FastNoise::NodeData>> nodeData;

    if (FastNoise::NodeData* firstNodeData = FastNoise::Metadata::DeserialiseNodeData(string, nodeData))
    {
        for (auto& data : nodeData)
        {
            FastNoise::NodeData* newNodeData = data.get();
            mNodes.emplace(std::piecewise_construct, std::forward_as_tuple(newNodeData),
                std::forward_as_tuple(*this, std::move(data)));
        }

        if (mNodes.size() == nodeData.size())
        {
            ChangeSelectedNode(firstNodeData);
        }

        Node& firstNode = mNodes.at(firstNodeData);

        ImNodes::SetNodeScreenSpacePos(firstNode.nodeId, nodePos);
        firstNode.AutoPositionChildNodes(nodePos);
        return true;
    }

    return false;
}

void NoiseEditor::DoHelp()
{
    ImGui::Text(" Help");
    if (ImGui::IsItemHovered())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
        ImGui::BeginTooltip();
        constexpr float alignPx = 110;

        ImGui::TextUnformatted("Add nodes");
        ImGui::SameLine(alignPx);
        ImGui::TextUnformatted("Right mouse click");

        ImGui::TextUnformatted("Pan graph");
        ImGui::SameLine(alignPx);
        ImGui::TextUnformatted("Right mouse drag");

        ImGui::TextUnformatted("Delete node/link");
        ImGui::SameLine(alignPx);
        ImGui::TextUnformatted("Backspace or Delete");

        ImGui::TextUnformatted("Node options");
        ImGui::SameLine(alignPx);
        ImGui::TextUnformatted("Right click node title");

        ImGui::EndTooltip();
        ImGui::PopStyleVar();
    }
}

void NoiseEditor::DoContextMenu()
{
    std::string className;
    ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    float distance = sqrtf(ImDot(drag, drag));
    bool openImportModal = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    if (distance < 5.0f && ImGui::BeginPopupContextWindow("new_node", 1))
    {
        mContextStartPos = ImGui::GetMousePosOnOpeningCurrentPopup();

        if (auto newMetadata = mContextMetadata.front()->DrawUI())
        {
            AddNode(mContextStartPos, newMetadata);
        }

        ImGui::EndPopup();
    }

    if (mDroppedLink)
    {
        ImGui::OpenPopup("new_node_drop");
        mDroppedLink = false;
    }
    if (ImGui::BeginPopup("new_node_drop",
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
        ImVec2 startPos = ImGui::GetMousePosOnOpeningCurrentPopup();

        auto newMetadata = mContextMetadata.front()->DrawUI(
            [](const FastNoise::Metadata* metadata)
            { return !metadata->memberNodeLookups.empty() || !metadata->memberHybrids.empty(); });

        if (newMetadata)
        {
            auto& newNode = AddNode(startPos, newMetadata);

            newNode.GetNodeLink(0) = mDroppedLinkNode;
            newNode.GeneratePreview();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

FastNoise::SmartNode<> NoiseEditor::GenerateSelectedPreview()
{
    auto find = mNodes.find(mSelectedNode);

    FastNoise::SmartNode<> generator;

    if (find != mNodes.end())
    {
        generator = FastNoise::NewFromEncodedNodeTree(find->second.serialised.c_str(), mMaxSIMDLevel);

        if (generator)
        {
            mActualSIMDLevel = generator->GetSIMDLevel();
        }
    }

    mNoiseTexture.ReGenerate(generator);

    return generator;
}

FastNoise::OutputMinMax NoiseEditor::GenerateNodePreviewNoise(FastNoise::Generator* gen, float* noise, float frequency, int seed)
{
    switch (mNodeGenType)
    {
    case NoiseTexture::GenType_2D:
        return gen->GenUniformGrid2D(noise, Node::NoiseSize / -2, Node::NoiseSize / -2, Node::NoiseSize, Node::NoiseSize,
            frequency, seed);

    case NoiseTexture::GenType_2DTiled:
        return gen->GenTileable2D(noise, Node::NoiseSize, Node::NoiseSize, frequency, seed);

    case NoiseTexture::GenType_Count:
        break;
    }

    return {};
}

NoiseEditor::Node* NoiseEditor::FindNodeFromId(int id)
{
    auto find = std::find_if(mNodes.begin(), mNodes.end(), [id](const auto& node) { return node.second.nodeId == id; });

    if (find != mNodes.end())
    {
        return &find->second;
    }

    return nullptr;
}

int NoiseEditor::GetFreeNodeId()
{
    static int newNodeId = 0;

    do
    {
        newNodeId = std::max(1, (newNodeId + 1) & (INT_MAX >> Node::AttributeBitCount));

    } while (FindNodeFromId(newNodeId));

    return newNodeId;
}

void NoiseEditor::ChangeSelectedNode(FastNoise::NodeData* newId)
{
    mSelectedNode = newId;
    auto find = mNodes.find(mSelectedNode);
    if (find != mNodes.end())
    {
        mNoiseTexture.SetFrequency(find->second.mNodeFrequency);
        mNoiseTexture.SetSeed(find->second.mNodeSeed);
    }

    FastNoise::SmartNode<> generator = GenerateSelectedPreview();

    // TODO : regenerate terrain mesh with newly selected noise node
    /*if (generator)
    {
        mMeshNoisePreview.ReGenerate(generator);
    }*/
}

const char* NoiseEditor::GetSIMDLevelName(FastSIMD::eLevel lvl)
{
    switch (lvl)
    {
    default:
    case FastSIMD::Level_Null:
        return "NULL";
    case FastSIMD::Level_Scalar:
        return "Scalar";
    case FastSIMD::Level_SSE:
        return "SSE";
    case FastSIMD::Level_SSE2:
        return "SSE2";
    case FastSIMD::Level_SSE3:
        return "SSE3";
    case FastSIMD::Level_SSSE3:
        return "SSSE3";
    case FastSIMD::Level_SSE41:
        return "SSE4.1";
    case FastSIMD::Level_SSE42:
        return "SSE4.2";
    case FastSIMD::Level_AVX:
        return "AVX";
    case FastSIMD::Level_AVX2:
        return "AVX2";
    case FastSIMD::Level_AVX512:
        return "AVX512";
    case FastSIMD::Level_NEON:
        return "NEON";
    }
}

/// <summary>
/// Get the current selected noise texture handle
/// </summary>
/// <returns></returns>
ResourceHandle<Image> NoiseEditor::GetCurrentNoiseTexture()
{
    return mNoiseTexture.GetNoiseTextureHandle();
}