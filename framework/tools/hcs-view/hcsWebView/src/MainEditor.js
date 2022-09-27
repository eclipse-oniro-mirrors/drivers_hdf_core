/*
* Copyright (c) 2022 Shenzhen Kaihong Digital Industry Development Co., Ltd. 
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at 
*
* http://www.apache.org/licenses/LICENSE-2.0 
*
* Unless required by applicable law or agreed to in writing, software 
* distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License. 
*/

const { XMessage } = require("./message/XMessage");
const { Lexer } = require("./hcs/lexer");
const { Generator } = require("./hcs/Generator");
const { Scr } = require("./engine/XDefine");
const { XButton } = require("./engine/control/XButton");
const { AttrEditor } = require("./AttrEditor");
const { NapiLog } = require("./hcs/NapiLog");
const { XSelect } = require("./engine/control/XSelect");
const { NodeTools, DataType } = require("./hcs/NodeTools");
const { ModifyNode } = require("./hcs/ModifyNode");

function rgba(r, g, b, a) {
    if (a == undefined) a = 255
    return a << 24 | r << 16 | g << 8 | b
}
class MainEditor {
    constructor() {
        this.files_ = {}
        this.nodeCount_ = {};
        this.filePoint_ = null;
        this.rootPoint_ = null;
        this.nodePoint_ = null;
        this.offX_ = 100;
        this.offY_ = 100;
        this.touchQueue_ = [];
        this.keyQueue_ = [];
        this.dropAll_ = {
            locked: false,
            oldx: -1,
            oldy: -1
        }
        this.nodeBtns = [];
        this.nodeMoreBtns = [];
        this.nodeBtnPoint_ = 0;
        this.nodeMoreBtnPoint_ = 0;
        XMessage.gi().registRecvCallback(this.onReceive);
        XMessage.gi().send("inited", "");
        AttrEditor.gi().freshEditor();

        this.sltInclude = new XSelect(["a", "b", "c"], "b");
        this.sltInclude.registCallback(this.onSelectInclude)
        NapiLog.registError(this.onError);
        this.errorMsg_ = []
        this.delay_ = 0;

        this.selectNode_ = {
            type: null,
            pnode: null
        };
        this.btnCancelSelect_ = new XButton();

        AttrEditor.gi().registCallback(this.onAttributeChange);

        this.mousePos_ = {
            x: 0,
            y: 0
        }
    }

    calcPostionY(data, y) {
        data.posY = y;
        let ty = y;
        switch (data.type_) {
            case 1://uint8
            case 2://uint16
            case 3://uint32
            case 4://uint64
                y += MainEditor.LINE_HEIGHT;
                break;
            case 5://string
                y += MainEditor.LINE_HEIGHT;
                break;
            case 6://ConfigNode
                for (let i in data.value_) {
                    y = this.calcPostionY(data.value_[i], y)
                }
                break;
            case 7://ConfigTerm
                y = this.calcPostionY(data.value_, y)
                break;
            case 8://Array class attribute value
                y += MainEditor.LINE_HEIGHT;
                break;
            case 9://Attribute equal to leaf
                y += MainEditor.LINE_HEIGHT
                break;
            case 10://Delete attribute
                y += MainEditor.LINE_HEIGHT
                break;
            case 11://bool
                y += MainEditor.LINE_HEIGHT
                break;
            default:
                NapiLog.logError("unknow" + data.type_);
                break;
        }
        if (y > ty) {
            data.posY = (ty + y - MainEditor.LINE_HEIGHT) / 2
        }
        return y > (ty + MainEditor.LINE_HEIGHT) ? y : (ty + MainEditor.LINE_HEIGHT);
    }
    getNodeText(data) {
        switch (data.nodeType_) {
            case 0://Data class nodes, not inheri
                return data.name_;
            case 3://Deletion class nodes
                return data.name_ + " : delete";
            case 4://Templete Class nodes
                return "templete " + data.name_;
            case 5://Data class nodes, inherit
                return data.name_ + ' :: ' + data.ref_;
            case 1://Copy class nodes
                return data.name_ + " : " + data.ref_;
            case 2://Reference modification class nodes
                return data.name_ + " : &" + data.ref_;
            default:
                return "unknow node type";
        }
    }

    drawNode(pm2f, s, size, x, y, color, borderColor, bkcolor) {
        let w = pm2f.getTextWidth(s, size);
        pm2f.drawRect(x - 3, y - 3, w + 20, 20 + 6, borderColor, 2);
        pm2f.fillRect(x - 3, y - 3, w + 20, 20 + 6, bkcolor);
        pm2f.drawText(s, size, x, y, 1, 1, 0, -1, -1, color);
        return w;
    }

    arrayNodeProc(w, pm2f, data, offx, offy) {
        let ss = "[" + data.value_.length + "]" + NodeTools.arrayToString(data)
        if (ss.length > 32) {
            ss = ss.substring(0, 29) + "..."
        }
        w = pm2f.drawText(ss, 18, offx, offy + data.posY, 1, 1, 0, -1, -1, 0xffffffff);
    }

    configNodeProc(w, pm2f, data, offx, offy, path) {
        this.setNodeButton(pm2f, offx, offy + data.posY, w, 20, path, data);

        if (data.value_.length > 0) {
            this.setNodeMoreButton(pm2f, offx, offy + data.posY, w, 20, data);
        }
        if (data.type_ == 6) {
            for (let i in data.value_) {
                if (data.value_[i].parent_.type_ == 6 && data.value_[i].parent_.isOpen_) {
                    this.drawObj(pm2f, data.value_[i], offx + w + 50, offy, path + ".");
                    pm2f.drawLine(data.posX + w, offy + data.posY + 10,
                        data.value_[i].posX, offy + data.value_[i].posY + 10, 0xffffffff, 2)
                } else if (data.value_[i].parent_.type_ == 7) {
                    this.drawObj(pm2f, data.value_[i], offx + w + 50, offy, path + ".");
                    pm2f.drawLine(data.posX + w, offy + data.posY + 10,
                        data.value_[i].posX, offy + data.value_[i].posY + 10, 0xffffffff, 2)
                }
                else {
                    NapiLog.logInfo("Node collapse does not need to draw child node");
                }
            }
        } else {
            for (let i in data.value_) {
                this.drawObj(pm2f, data.value_[i], offx + w + 50, offy, path + ".");
                pm2f.drawLine(data.posX + w, offy + data.posY + 10,
                    data.value_[i].posX, offy + data.value_[i].posY + 10, 0xffffffff, 2)
            }
        }
    }
    drawObj(pm2f, data, offx, offy, path) {
        let w;
        path += data.name_;
        data.posX = offx;

        switch (data.type_) {
            case 1://uint8
            case 2://uint16
            case 3://uint32
            case 4://uint64
                w = pm2f.drawText(NodeTools.jinZhi10ToX(data.value_, data.jinzhi_),
                    18, offx, offy + data.posY, 1, 1, 0, -1, -1, 0xE6000000);
                break;
            case 5://string
                w = pm2f.drawText('"' + data.value_ + '"', 18, offx, offy + data.posY, 1, 1, 0, -1, -1, 0xE6000000);
                break;
            case 6://ConfigNode
                var color = data.errMsg_ != null ? 0xE6FF0000 : 0xE6000000;
                w = this.drawNode(pm2f, this.getNodeText(data), 18, offx, offy + data.posY,
                color, rgba(196, 196, 196), 0xffffffff);
                this.configNodeProc(w, pm2f, data, offx, offy, path)
                break;
            case 7://ConfigTerm
                w = this.drawNode(pm2f, data.name_ + "=", 18, offx, offy + data.posY, 0xE6000000, rgba(244,145,38), 0xffffffff);
                this.setNodeButton(pm2f, offx, offy + data.posY, w, 20, path, data);
                this.drawObj(pm2f, data.value_, offx + w, offy, path);
                break;
            case 8://Array class attribute value
                this.arrayNodeProc(w, pm2f, data, offx, offy);
                break;
            case 9://Attribute equal to leaf
                w = pm2f.drawText("&" + data.value_, 18, offx, offy + data.posY, 1, 1, 0, -1, -1, 0xffff0000);
                break;
            case 10://Delete attribute
                w = pm2f.drawText("delete", 18, offx, offy + data.posY, 1, 1, 0, -1, -1, 0xffff0000);
                break;
            case 11://bool
                if (data.value_) w = pm2f.drawText("true", 18, offx, offy + data.posY, 1, 1, 0, -1, -1, 0xffff0000);
                else w = pm2f.drawText("false", 18, offx, offy + data.posY, 1, 1, 0, -1, -1, 0xffff0000);
                break;
            default:
                NapiLog.logError("unknow" + data.type_);
                break;
        }
        if (data.errMsg_ != null) {
            if (parseInt(this.delay_ / 10) % 2 == 0) {
                pm2f.drawRect(offx - 5, offy + data.posY - 5, w + 10, 20 + 10, 0xffff0000, 3);
            }
            pm2f.drawText(data.errMsg_, 18, offx + w - 5, offy + data.posY + 5, 1, 1, 0, -1, -3, 0xffff0000);
        }
    }
    
    setNodeButton(pm2f, x, y, w, h, path, node) {
        if (this.nodePoint_ == node) {
            pm2f.drawRect(x - 3, y - 3, w + 20, h + 6, 0xff487EB8, 3);
        }
        if (this.nodeBtnPoint_ >= this.nodeBtns.length) {
            this.nodeBtns.push(new XButton());
        }
        let pbtn = this.nodeBtns[this.nodeBtnPoint_];
        pbtn.move(x - 3, y - 3, w + 6, h + 6);
        pbtn.name_ = path;
        pbtn.node_ = node;
        this.nodeBtnPoint_ += 1;
    }

    setNodeMoreButton(pm2f, x, y, w, h, node) {
        if (this.nodeMoreBtnPoint_ >= this.nodeMoreBtns.length) {
            this.nodeMoreBtns.push(new XButton());
        }
        let pbtn = this.nodeMoreBtns[this.nodeMoreBtnPoint_];
        pbtn.move(x + w + 18, y - 3, 15, h + 6);
        pbtn.draw();
        pm2f.drawLine(x + w + 20, y + 10, x + w + 30, y + 10, 0xffffffff, 2)
        if(!node.isOpen_){
            pm2f.drawLine(x + w + 25, y + 5, x + w + 25, y + 15, 0xffffffff, 2)
        }
        pbtn.node_ = node;
        this.nodeMoreBtnPoint_ += 1;
    }

    draw(pm2f) {
        pm2f.fillRect(0, 0, Scr.logicw, Scr.logich, 0xff303030);

        if (this.filePoint_ != null && this.filePoint_ in this.files_) {
            let data = this.files_[this.filePoint_];
            this.calcPostionY(data, 0);

            this.nodeBtnPoint_ = 0;
            this.nodeMoreBtnPoint_ = 0;
            this.drawObj(pm2f, data, this.offX_, this.offY_, "");
        }
        this.sltInclude.move(10, 10, 200, 20).draw();

        if (this.selectNode_.type != null) {
            if (this.selectNode_.type == "change_target") {
                pm2f.drawText("点击选择目标", 18, this.mousePos_.x, this.mousePos_.y, 1, 1, 0, -3, -3, 0xff487EB8);
                this.btnCancelSelect_.name_ = "取消选择";
            }
            else if (this.selectNode_.type == "copy_node") {
                pm2f.drawText("已复制" + this.selectNode_.pnode.name_, 18, this.mousePos_.x, this.mousePos_.y, 1,
                    1, 0, -3, -3, 0xff487EB8);
                this.btnCancelSelect_.name_ = "取消复制";
            }
            else if (this.selectNode_.type == "cut_node") {
                pm2f.drawText("已剪切" + this.selectNode_.pnode.name_, 18, this.mousePos_.x, this.mousePos_.y, 1,
                    1, 0, -3, -3, 0xff487EB8);
                this.btnCancelSelect_.name_ = "取消剪切";
            }

            this.btnCancelSelect_.move(Scr.logicw - 250, Scr.logich - 30, 100, 20).draw();
        }

        if (this.errorMsg_.length > 0) {
            let ts = (new Date()).getTime()
            while (this.errorMsg_.length > 0 && this.errorMsg_[0][0] < ts) {
                this.errorMsg_.shift();
            }
            for (let i in this.errorMsg_) {
                let y = Scr.logich / 2 - this.errorMsg_.length * 20 + i * 20
                let a = parseInt((this.errorMsg_[i][0] - ts) / 2);
                if (a > 255) a = 255;
                NapiLog.logError(a);
                a = a << 24;
                pm2f.fillRect(0, y, Scr.logicw, 20, 0xff0000 | a);
                pm2f.drawText(this.errorMsg_[i][1], 18, Scr.logicw / 2, y, 1, 1, 0, -2, -1, 0xffffff | a);
            }

        }
        this.delay_ += 1;
        this.procAll();
    }

    buttonClickedProc(nodeBtns) {
        if (this.selectNode_.type == null ||
            (this.selectNode_.type == "copy_node" || this.selectNode_.type == "cut_node")) {
            this.nodePoint_ = nodeBtns.node_;
            AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
            return true;
        }
        //What can be copied: data does not inherit, copy class nodes, template class nodes and data class inheritance
        if (this.selectNode_.type == "change_target") {
            let pn = nodeBtns.node_;
            if (pn.type_ == DataType.NODE) {
                if (this.selectNode_.pnode.type_ == DataType.NODE) {
                    if (NodeTools.getPathByNode(this.selectNode_.pnode.parent_) ==
                        NodeTools.getPathByNode(pn.parent_)) {
                        this.selectNode_.pnode.ref_ = pn.name_;
                    }
                    else {
                        this.selectNode_.pnode.ref_ = NodeTools.getPathByNode(pn);
                    }
                }
                else if (this.selectNode_.pnode.type_ == DataType.REFERENCE) {
                    if (NodeTools.getPathByNode(this.selectNode_.pnode.parent_.parent_) ==
                        NodeTools.getPathByNode(pn.parent_)) {
                        this.selectNode_.pnode.value_ = pn.name_;
                    }
                    else {
                        this.selectNode_.pnode.value_ = NodeTools.getPathByNode(pn);
                    }
                }

                this.selectNode_.type = null;
                AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
                this.onAttributeChange("writefile");
            }
        }
        return true
    }

    dropAllLocked(msg, x, y) {
        if (msg == 2) {
            this.offX_ += (x - this.dropAll_.oldx);
            this.offY_ += (y - this.dropAll_.oldy);
            this.dropAll_.oldx = x;
            this.dropAll_.oldy = y;
        }
        if (msg == 3) {
            this.dropAll_.locked = false;
        }
    }

    procTouch(msg, x, y) {
        this.mousePos_.x = x;
        this.mousePos_.y = y;
        if (this.dropAll_.locked) {
            this.dropAllLocked(msg, x, y);
            return true;
        }

        if (this.sltInclude.procTouch(msg, x, y)) {
            return true;
        }

        if (this.selectNode_.type != null) {
            if (this.btnCancelSelect_.procTouch(msg, x, y)) {
                if (this.btnCancelSelect_.isClicked()) {
                    this.selectNode_.type = null;
                }
                return true;
            }
        }

        for (let i = 0; i < this.nodeBtnPoint_; i++) {
            if (this.nodeBtns[i].procTouch(msg, x, y)) {
                let nodeBtns = this.nodeBtns[i];
                if (nodeBtns.isClicked()) {
                    this.buttonClickedProc(nodeBtns);
                }
                return true;
            }
        }

        for (let i = 0; i < this.nodeMoreBtnPoint_; i++) {
            if (this.nodeMoreBtns[i].procTouch(msg, x, y)) {
                let nodeMoreBtn = this.nodeMoreBtns[i];
                if (nodeMoreBtn.isClicked()) {
                    this.buttonClickedProc(nodeMoreBtn);
                    this.nodeMoreBtns[i].node_.isOpen_ = !this.nodeMoreBtns[i].node_.isOpen_;
                }
                return true;
            }
        }
        
        //Drag screen
        if (msg == 1 && !this.dropAll_.locked) {
            this.dropAll_.locked = true;
            this.dropAll_.oldx = x;
            this.dropAll_.oldy = y;
            return true;
        }
    }
    procKey(k) {
        if (k == "ctrl+c") {
            if (this.nodePoint_ != null) {
                this.selectNode_ = {
                    type: "copy_node",
                    pnode: this.nodePoint_
                };
            }
        }
        else if (k == "ctrl+x") {
            if (this.nodePoint_ != null) {
                this.selectNode_ = {
                    type: "cut_node",
                    pnode: this.nodePoint_
                };
            }
        }
        else if (k == "ctrl+v") {
            if (this.selectNode_.type != null && this.nodePoint_ != null) {
                let parent = this.nodePoint_
                if (this.nodePoint_.type_ != DataType.NODE)
                    parent = this.nodePoint_.parent_;
                parent.value_.push(NodeTools.copyNode(this.selectNode_.pnode, parent))
                if (this.selectNode_.type == "cut_node") {
                    ModifyNode.deleteNode(this.selectNode_.pnode)
                    this.selectNode_.type = null;
                }
                this.checkAllError();
            }
        }
        else if (k == "Delete") {
            if (this.nodePoint_ != null) {
                ModifyNode.deleteNode(this.nodePoint_)
                AttrEditor.gi().freshEditor();
            }
        }
    }
    procAll() {
        while (this.touchQueue_.length > 0) {
            let touch = this.touchQueue_.shift();
            this.procTouch(touch[0], touch[1], touch[2]);
        }

        while (this.keyQueue_.length > 0) {
            let k = this.keyQueue_.shift();
            this.procKey(k);
        }
    }
    onSelectInclude(sel) {
        MainEditor.gi().filePoint_ = sel;
        AttrEditor.gi().freshEditor();
    }

    nodeCount(data) {
        let ret = 1;
        switch (data.type_) {
            case 1://uint8
            case 2://uint16
            case 3://uint32
            case 4://uint64
            case 5://string
                break;
            case 6://ConfigNode
                for (let i in data.value_) {
                    ret += this.nodeCount(data.value_[i]);
                }
                break;
            case 7://ConfigTerm
                ret += this.nodeCount(data.value_);
                break;
            case 8://Array class attribute value
            case 9://Attribute equal to leaf
            case 10://Delete attribute
            case 11://bool
                break;
            default:
                NapiLog.logError("unknow" + data.type_);
                break;
        }
        return ret;
    }
    isNodeCountChanged(fn) {
        if (!(fn in this.nodeCount_)) {
            this.nodeCount_[fn] = -1;
        }
        let newcount = this.nodeCount(this.files_[fn]);
        if (this.nodeCount_[fn] != newcount) {
            this.nodeCount_[fn] = newcount;
            return true;
        }
        return false;
    }
    onAttributeChange(type, value) {
        let pme = MainEditor.gi();
        if (type == "writefile") {
            let data1 = Generator.gi().makeHcs(pme.filePoint_, pme.files_[pme.filePoint_])
            if (pme.isNodeCountChanged(pme.filePoint_)) {
                let data2 = []
                for (let j in data1) {
                    data2.push(data1.charCodeAt(j))
                }
                Lexer.FILE_AND_DATA[pme.filePoint_] = data2;
                pme.parse(pme.filePoint_)
                let t=NodeTools.getPathByNode(pme.nodePoint_);
                if(t){
                    pme.nodePoint_=NodeTools.getNodeByPath(pme.files_[pme.filePoint_],t)
                }
                else pme.nodePoint_= null;
                
                // pme.nodePoint_ = null;
                AttrEditor.gi().freshEditor(pme.filePoint_, pme.nodePoint_);
            }
            pme.checkAllError();
            XMessage.gi().send("writefile", {
                fn: pme.filePoint_,
                data: data1
            })
        }
        else if (type.substring(0, 13) == "change_target") {
            pme.selectNode_.type = type;
            pme.selectNode_.pnode = value;
        }
    }
    onError(msg) {
        // MainEditor.gi().errorMsg_.push([(new Date()).getTime() + 5000, msg])
    }
    onTouch(msg, x, y) {
        this.touchQueue_.push([msg, x, y]);
    }
    onKey(k) {
        this.keyQueue_.push(k)
    }
    onReceive(type, data) {
        NapiLog.logError(type);
        let me = MainEditor.gi();
        if (type == "parse") {
            me.parse(data)
        }
        else if (type == "filedata") {
            Lexer.FILE_AND_DATA[data.fn] = data.data;
            me.parse(data.fn)
        }
        else if (type == "freshfiledata") {
            Lexer.FILE_AND_DATA[data.fn] = data.data;
        }
    }
    parse(fn) {
        if (this.rootPoint_ == null) {
            this.rootPoint_ = fn
        }
        let t = Generator.gi().hcsToAst(fn);
        if (!t) return;

        let fs = []
        for (let i in t) {
            this.files_[i] = Generator.gi().astToObj(t[i].ast.astRoot_);
            fs.push(i)
        }
        this.filePoint_ = this.rootPoint_;
        this.sltInclude.resetList(fs, this.filePoint_)
        AttrEditor.gi().setFiles(this.files_)

        this.checkAllError();
    }
    checkAllError() {
        NapiLog.clearError()
        let n1 = Generator.gi().mergeObj(this.files_)
        if (n1) {
            n1 = Generator.gi().expandObj(n1)
            if (NapiLog.getResult()[0])
                return true;
        }
        return false;
    }

}
MainEditor.LINE_HEIGHT = 30

MainEditor.pInstance_ = null;
MainEditor.gi = function () {
    if (MainEditor.pInstance_ == null) MainEditor.pInstance_ = new MainEditor();
    return MainEditor.pInstance_;
}

module.exports = {
    MainEditor
}