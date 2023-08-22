/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

const { XMessage } = require('./message/XMessage');
const { Lexer } = require('./hcs/lexer');
const { Generator } = require('./hcs/Generator');
const { Scr } = require('./engine/XDefine');
const { XButton } = require('./engine/control/XButton');
const { AttrEditor } = require('./AttrEditor');
const { NapiLog } = require('./hcs/NapiLog');
const { XSelect } = require('./engine/control/XSelect');
const { NodeTools, DataType, NodeType } = require('./hcs/NodeTools');
const { ModifyNode } = require('./hcs/ModifyNode');
const { CanvasInput } = require('./hcs/CanvasInput');
const { RightMenu } = require('./engine/RightMenu');
const { XTexture } = require('./engine/graphics/XTexture');
const { XTools } = require('./engine/XTools');

const { ObjectType } = require('./hcs/ast');
var DISPLAY_TEXT_MAX = 30;
var ELLIPSIS_LEN = 3;
var EQUAL_SIGN_LEN = 3;
const MAX_RANDOM = 255;

function rgba(colorArr) {
  return 0xff000000 | (colorArr[0] << 16) | (colorArr[1] << 8) | colorArr[2];
}

function getDarker(colorArr) {
  for (let i = 0; i < colorArr.length; ++i) {
    if (colorArr[i] - 0 > 10) {
      colorArr[i] = colorArr[i] - 10;
    }
  }
  return rgba(colorArr);
}

function isDarkColor(colorArr) {
  let grayLevel =
    colorArr[0] * 0.299 + colorArr[1] * 0.587 + colorArr[2] * 0.114;
  return grayLevel < 192;
}

function getVsCodeTheme() {
  MainEditor.CANVAS_BG = 0xff272727;
  MainEditor.CANVAS_LINE = 0xff000000;
  MainEditor.NODE_TEXT_COLOR = 0xffffffff;
  let canvasBg = document.getElementById('canvas_bg');
  var bgStyle = document.defaultView.getComputedStyle(canvasBg, null);
  var bgColor = bgStyle.getPropertyValue('background-color').match(/\d{1,3}/g);
  if (bgColor) {
    MainEditor.CANVAS_BG = rgba(bgColor);
    MainEditor.CANVAS_LINE = getDarker(bgColor);
    MainEditor.IS_DARK_BG = isDarkColor(bgColor);
    RightMenu.isDarkBackground_ = MainEditor.IS_DARK_BG;
  }

  var txtColor = bgStyle.getPropertyValue('color').match(/\d{1,3}/g);
  if (txtColor) {
    MainEditor.NODE_TEXT_COLOR = rgba(txtColor);
  }
}

class MainEditor {
  constructor() {
    this.files_ = {};
    this.nodeCount_ = {};
    this.filePoint_ = null;
    this.rootPoint_ = null;
    this.nodePoint_ = null;
    this.offX_ = 100;
    this.offY_ = 100;
    this.searchKey = null;
    this.touchQueue_ = [];
    this.keyQueue_ = [];
    this.dropAll_ = {
      locked: false,
      oldx: -1,
      oldy: -1,
    };
    getVsCodeTheme();
    this.nodeBtns = [];
    this.nodeMoreBtns = [];
    this.nodeBtnPoint_ = 0;
    this.nodeMoreBtnPoint_ = 0;
    XMessage.gi().registRecvCallback(this.onReceive);
    XMessage.gi().send('inited', '');
    AttrEditor.gi().freshEditor();

    this.sltInclude = new XSelect(['a', 'b', 'c'], 'b');
    this.sltInclude.registCallback(this.onSelectInclude);
    NapiLog.registError(this.onError);
    this.errorMsg_ = [];
    this.cutImgDict_ = {};
    this.whiteImg_ = -1;
    this.whiteCut_ = -1;
    this.cicleImg_ = -1;
    this.circleCut_ = -1;
    this.cicleOpenImg_ = -1;
    this.circleOpenCut_ = -1;
    this.rectangleFocusImg_ = -1;
    this.rectangleFocusCut_ = -1;
    this.nodeIconImg_ = -1;
    this.nodeIconCut_ = -1;
    this.attrIconImg_ = -1;
    this.attrIconCut_ = -1;
    this.rootIconImg_ = -1;
    this.rootIconCut_ = -1;
    this.rootIconFocusImg_ = -1;
    this.rootIconFocusCut_ = -1;
    RightMenu.backgroundImg_ = -1;
    RightMenu.backgroundCut_ = -1;
    RightMenu.popItemFocusImg_ = -1;
    RightMenu.popItemFocusCut_ = -1;
    this.leftRectCicleCut_ = -1;
    this.centerRectCut_ = -1;
    this.rightRectCicleCut_ = -1;
    this.leftRectFocusCicleCut_ = -1;
    this.centerFocusCut_ = -1;
    this.rightRectFocusCicleCut_ = -1;
    this.delay_ = 0;
    this.searchBgImg_ = -1;
    this.searchBgCut_ = -1;
    this.upImg_ = -1;
    this.upCut_ = -1;
    this.downImg_ = -1;
    this.downCut_ = -1;
    this.closeImg_ = -1;
    this.closeCut_ = -1;
    this.searchImg_ = -1;
    this.searchCut_ = -1;
    this.isSearchResult_ = false;
    this.searchRectFocusCicleImg_ = -1;
    this.leftSearchFocusCicleCut_ = -1;
    this.centerSearchCut_ = -1;
    this.rightSearchFocusCicleCut_ = -1;

    this.searchAttrCicleImg_ = -1;
    this.leftSearchAttrCicleCut_ = -1;
    this.centerSearchAttrCut_ = -1;
    this.rightSearchAttrCicleCut_ = -1;

    this.selectNode_ = {
      type: null,
      pnode: null,
    };
    this.btnCancelSelect_ = new XButton();

    AttrEditor.gi().registCallback(this.onAttributeChange);

    this.mousePos_ = {
      x: 0,
      y: 0,
    };

    this.whiteImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.whiteCut_ = XTexture.gi().makeCut(
      this.whiteImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_['whiteCut'] = 'rectangle.png';

    this.cicleImg_ = XTexture.gi().loadTextureFromImage('../images/circle.png');
    this.circleCut_ = XTexture.gi().makeCut(
      this.cicleImg_,
      0,
      0,
      20,
      20,
      20,
      20
    );
    this.cutImgDict_['circleCut'] = 'circle.png';

    this.cicleOpenImg_ = XTexture.gi().loadTextureFromImage(
      '../images/circle_open.png'
    );
    this.circleOpenCut_ = XTexture.gi().makeCut(
      this.cicleOpenImg_,
      0,
      0,
      20,
      20,
      20,
      20
    );
    this.cutImgDict_['circleOpenCut'] = 'circle_open.png';

    this.rectangleFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.rectangleFocusCut_ = XTexture.gi().makeCut(
      this.rectangleFocusImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_['rectangleFocusCut'] = 'rectangle_focus.png';

    this.nodeIconImg_ = XTexture.gi().loadTextureFromImage(
      '../images/node_icon.png'
    );
    this.nodeIconCut_ = XTexture.gi().makeCut(
      this.nodeIconImg_,
      0,
      0,
      8,
      8,
      8,
      8
    );
    this.cutImgDict_['nodeIconCut'] = 'node_icon.png';

    this.attrIconImg_ = XTexture.gi().loadTextureFromImage(
      '../images/attribute_icon.png'
    );
    this.attrIconCut_ = XTexture.gi().makeCut(
      this.attrIconImg_,
      0,
      0,
      8,
      8,
      8,
      8
    );
    this.cutImgDict_['attrIconCut'] = 'attribute_icon.png';

    this.rootIconImg_ = XTexture.gi().loadTextureFromImage(
      '../images/root_btn.png'
    );
    this.rootIconCut_ = XTexture.gi().makeCut(
      this.rootIconImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_['rootIconCut'] = 'root_btn.png';

    this.rootIconFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/root_btn_focus.png'
    );
    this.rootIconFocusCut_ = XTexture.gi().makeCut(
      this.rootIconFocusImg_,
      0,
      0,
      132,
      32,
      132,
      32
    );
    this.cutImgDict_['rootIconFocusCut'] = 'root_btn_focus.png';

    this.leftRectCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.leftRectCicleCut_ = XTexture.gi().makeCut(
      this.leftRectCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerRectImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.centerRectCut_ = XTexture.gi().makeCut(
      this.centerRectImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightRectCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle.png'
    );
    this.rightRectCicleCut_ = XTexture.gi().makeCut(
      this.rightRectCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );

    this.leftRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.leftRectFocusCicleCut_ = XTexture.gi().makeCut(
      this.leftRectFocusCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.centerFocusCut_ = XTexture.gi().makeCut(
      this.centerFocusImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/rectangle_focus.png'
    );
    this.rightRectFocusCicleCut_ = XTexture.gi().makeCut(
      this.rightRectFocusCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );

    let bgPic = RightMenu.isDarkBackground_ ? 'pop_background.png' : 'pop_background_light.png';
    let bgPicPath = '../images/' + bgPic;
    RightMenu.backgroundImg_ = XTexture.gi().loadTextureFromImage(bgPicPath);
    RightMenu.backgroundCut_ = XTexture.gi().makeCut(
      RightMenu.backgroundImg_,
      0,
      0,
      156,
      112,
      156,
      112
    );
    this.cutImgDict_['backgroundCut'] = bgPic;

    RightMenu.popItemFocusImg_ = XTexture.gi().loadTextureFromImage(
      '../images/pop_item_focus.png'
    );
    RightMenu.popItemFocusCut_ = XTexture.gi().makeCut(
      RightMenu.popItemFocusImg_,
      0,
      0,
      148,
      32,
      148,
      32
    );
    this.cutImgDict_['popItemFocusCut'] = 'pop_item_focus.png';

    this.searchBgImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search_bg.png'
    );
    this.searchBgCut_ = XTexture.gi().makeCut(
      this.searchBgImg_,
      0,
      0,
      494,
      56,
      494,
      56
    );
    this.cutImgDict_['searchBgCut'] = 'search_bg.png';

    this.upImg_ = XTexture.gi().loadTextureFromImage(
      '../images/chevron-up.png'
    );
    this.upCut_ = XTexture.gi().makeCut(this.upImg_, 0, 0, 16, 16, 16, 16);
    this.cutImgDict_['upCut'] = 'chevron-up.png';

    this.downImg_ = XTexture.gi().loadTextureFromImage(
      '../images/chevron-down.png'
    );
    this.downCut_ = XTexture.gi().makeCut(this.downImg_, 0, 0, 16, 16, 16, 16);
    this.cutImgDict_['downCut'] = 'chevron-down.png';

    this.closeImg_ = XTexture.gi().loadTextureFromImage('../images/close.png');
    this.closeCut_ = XTexture.gi().makeCut(
      this.closeImg_,
      0,
      0,
      16,
      16,
      16,
      16
    );
    this.cutImgDict_['closeCut'] = 'close.png';

    this.searchImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search.png'
    );
    this.searchCut_ = XTexture.gi().makeCut(
      this.searchImg_,
      0,
      0,
      16,
      16,
      16,
      16
    );
    this.cutImgDict_['searchCut'] = 'search.png';

    this.searchRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search_nood_rect.png'
    );
    this.leftSearchFocusCicleCut_ = XTexture.gi().makeCut(
      this.searchRectFocusCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerSearchCut_ = XTexture.gi().makeCut(
      this.searchRectFocusCicleImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightSearchFocusCicleCut_ = XTexture.gi().makeCut(
      this.searchRectFocusCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );
    this.cutImgDict_['searchNoodRectImg'] = 'search_nood_rect.png';

    this.searchAttrCicleImg_ = XTexture.gi().loadTextureFromImage(
      '../images/search_attr_rect.png'
    );
    this.leftSearchAttrCicleCut_ = XTexture.gi().makeCut(
      this.searchAttrCicleImg_,
      0,
      0,
      8,
      32,
      132,
      32
    );
    this.centerSearchAttrCut_ = XTexture.gi().makeCut(
      this.searchAttrCicleImg_,
      8,
      0,
      116,
      32,
      132,
      32
    );
    this.rightSearchAttrCicleCut_ = XTexture.gi().makeCut(
      this.searchAttrCicleImg_,
      124,
      0,
      8,
      32,
      132,
      32
    );
    this.cutImgDict_['searchAttrRectImg'] = 'search_attr_rect.png';

    XMessage.gi().send('cutImgDict', {
      data: this.cutImgDict_,
    });
    this.modifyPos_ = null;
    this.isFirstDraw = true;
    this.lenHierarchy = 0;

    this.searchInput = null;
    this.historyZ = [];
    this.historyBase = {};
    this.historyPushed = false;
  }

  reloadMenuBgPic() {
    let bgPic = RightMenu.isDarkBackground_ ? 'pop_background.png' : 'pop_background_light.png';
    let bgPicPath = '../images/' + bgPic;
    RightMenu.backgroundImg_ = XTexture.gi().loadTextureFromImage(bgPicPath);
    RightMenu.backgroundCut_ = XTexture.gi().makeCut(
      RightMenu.backgroundImg_,
      0,
      0,
      156,
      112,
      156,
      112
    );
    this.cutImgDict_['backgroundCut'] = bgPic;
    XMessage.gi().send('reloadMenuBg', {
      data: bgPic,
    });
  }

  calcPostionY(data, y) {
    data.posY = y;
    let ty = y;
    switch (data.type_) {
      case 1:
      case 2:
      case 3:
      case 4:
        y += MainEditor.LINE_HEIGHT;
        break;
      case 5:
        y += MainEditor.LINE_HEIGHT;
        break;
      case 6:
        if (!data.isOpen_) {
          y += MainEditor.LINE_HEIGHT;
        } else {
          for (let i in data.value_) {
            y = this.calcPostionY(data.value_[i], y);
          }
        }
        break;
      case 7:
        y = this.calcPostionY(data.value_, y);
        break;
      case 8:
        y += MainEditor.LINE_HEIGHT;
        break;
      case 9:
        y += MainEditor.LINE_HEIGHT;
        break;
      case 10:
        y += MainEditor.LINE_HEIGHT;
        break;
      case 11:
        y += MainEditor.LINE_HEIGHT;
        break;
      default:
        NapiLog.logError('unknow' + data.type_);
        break;
    }
    if (y > ty) {
      data.posY = (ty + y - MainEditor.LINE_HEIGHT) / 2;
    }
    return y > ty + MainEditor.LINE_HEIGHT ? y : ty + MainEditor.LINE_HEIGHT;
  }

  getNodeText(data) {
    switch (data.nodeType_) {
      case 0:
        return data.name_;
      case 3:
        return data.name_ + ' : delete';
      case 4:
        return 'templete ' + data.name_;
      case 5:
        if (data.ref_ == 'unknow') {
          return data.name_;
        }
        return data.name_ + ' :: ' + data.ref_;
      case 1:
        if (data.ref_ == 'unknow') {
          return data.name_;
        }
        return data.name_ + ' : ' + data.ref_;
      case 2:
        if (data.ref_ == 'unknow') {
          return data.name_;
        }
        return data.name_ + ' : &' + data.ref_;
      default:
        return 'unknow node type';
    }
  }

  drawNode(pm2f, s, size, x, y, type, data) {
    const SPACE = 2;
    const MAXLEN_DISPLAY_WITH_SPACE = 25;
    const MAXLEN_DISPLAY_NO_SPACE = 26;
    const DISPLAY_TEXT_MAX_NOPOINT = 27;
    let w = pm2f.getTextWidth(type == DataType.ATTR ? s + ' = ' : s, size);
    if (data.parent_ == undefined) {
      return w;
    }

    if (type == DataType.ATTR) {
      let lenDisplay = DISPLAY_TEXT_MAX - EQUAL_SIGN_LEN;
      if (s.length < MAXLEN_DISPLAY_WITH_SPACE) {
        pm2f.drawText(s, size, x - (data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        pm2f.drawText(' = ', size, x - (data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE + pm2f.getTextWidth(s, size),
        y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 0, 0, 0xffa9a9a9);
      } else if (s.length == MAXLEN_DISPLAY_WITH_SPACE) {
        pm2f.drawText(s, size, x - (data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        pm2f.drawText(' =', size, x - (data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE + pm2f.getTextWidth(s, size),
        y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 0, 0, 0xffa9a9a9);
      } else if (s.length == MAXLEN_DISPLAY_NO_SPACE) {
        pm2f.drawText(s, size, x - (data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        pm2f.drawText('=', size, x - (data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE + pm2f.getTextWidth(s, size),
        y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 0, 0, 0xffa9a9a9);
      } else if (s.length > MAXLEN_DISPLAY_NO_SPACE) {
        s = s.substring(0, lenDisplay) + '...';
        pm2f.drawText(s, size, x - (data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0) +
          MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * SPACE,
        y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
      }
    } else {
      pm2f.drawText( s.length > DISPLAY_TEXT_MAX ? s.substring(0, DISPLAY_TEXT_MAX_NOPOINT) + '...' : s, size, x -
        (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_) + MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE * 2,
      y + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    }
    return w;
  }

  drawBrokenLine(pm2f, data, offy, i) {
    let dis =
      data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0;
    let baseX_ =
      data.posX +
      MainEditor.NODE_RECT_WIDTH -
      (MainEditor.NODE_RECT_WIDTH - data.nodeWidth_) +
      MainEditor.NODE_TEXT_OFFX +
      MainEditor.NODE_MORE_CHILD -
      dis;
    pm2f.drawLine(
      baseX_,
      offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2,
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2,
      MainEditor.NODE_LINE_COLOR,
      0.5
    );

    pm2f.drawLine(
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2,
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.value_[i].posY + MainEditor.NODE_RECT_HEIGHT / 2,
      MainEditor.NODE_LINE_COLOR,
      0.5
    );

    pm2f.drawLine(
      baseX_ + MainEditor.LINE_WIDTH,
      offy + data.value_[i].posY + MainEditor.NODE_RECT_HEIGHT / 2,
      baseX_ + MainEditor.LINE_WIDTH * 2,
      offy + data.value_[i].posY + MainEditor.NODE_RECT_HEIGHT / 2,
      MainEditor.NODE_LINE_COLOR,
      0.5
    );
  }

  arrayNodeProc(w, pm2f, data, offx, offy) {
    let ss = '[' + data.value_.length + ']' + NodeTools.arrayToString(data);
    let keyAndValue = data.parent_.name_ + ' = ';
    if (keyAndValue.length >= 30) {
      return;
    } else if (keyAndValue.length == 29) {
      w = pm2f.drawText('.', 14, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    } else if (keyAndValue.length == 28) {
      w = pm2f.drawText('..', 14, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    } else if (keyAndValue.length == 27) {
      w = pm2f.drawText('...', 14, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    } else if (keyAndValue.length < 27) {
      let displayValueLen = DISPLAY_TEXT_MAX - keyAndValue.length;
      if (ss.length > displayValueLen) {
        ss = ss.substring(0, displayValueLen - 3) + '...';
      }
      w = pm2f.drawText(ss, 14, offx, offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
    }
  }

  configNodeProc(w, pm2f, data, offx, offy, path) {
    let dis = data.parent_ != undefined ? MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_ : 0;
    this.setNodeButton(pm2f, offx, offy + data.posY, w, MainEditor.NODE_TEXT_SIZE, path, data);
    if (data.value_.length > 0) {
      this.setNodeMoreButton(pm2f, offx - dis, offy + data.posY, MainEditor.NODE_MORE_CHILD, MainEditor.NODE_MORE_CHILD, data);
    }
    let drawNodeX_ = offx + MainEditor.NODE_RECT_WIDTH + MainEditor.NODE_SIZE_BG_OFFX + MainEditor.NODE_MORE_CHILD + MainEditor.LINE_WIDTH * 2 - dis;
    if (data.type_ == DataType.NODE) {
      for (let i in data.value_) {
        if (
          data.value_[i].parent_.type_ == 6 &&
          data.value_[i].parent_.isOpen_
        ) {
          this.drawObj(pm2f, data.value_[i], drawNodeX_, offy, path + '.');
          this.drawBrokenLine(pm2f, data, offy, i);
        } else if (data.value_[i].parent_.type_ == DataType.ATTR) {
          this.drawObj(pm2f, data.value_[i], drawNodeX_, offy, path + '.');
          pm2f.drawLine(data.posX + w, offy + data.posY + 10,
          data.value_[i].posX, offy + data.value_[i].posY + 10, MainEditor.NODE_TEXT_COLOR, 1);
        } else {
          NapiLog.logInfo('Node collapse does not need to draw child node');
        }
      }
    } else {
      for (let i in data.value_) {
        this.drawObj(pm2f, data.value_[i], drawNodeX_, offy, path + '.');
        pm2f.drawLine(data.posX + w, offy + data.posY + 10,
        data.value_[i].posX, offy + data.value_[i].posY + 10, MainEditor.NODE_TEXT_COLOR, 1);
      }
    }
  }

  drawObj(pm2f, data, offx, offy, path) {
    let w;
    path += data.name_;
    data.posX = offx;
    if (this.maxX < offx) {
      this.maxX = offx;
    }
    let parentTextWidth = pm2f.getTextWidth(' = ', MainEditor.NODE_TEXT_SIZE);
    let drawTextX_ = offx + MainEditor.LOGO_LEFT_PADDING + MainEditor.LOGO_SIZE + parentTextWidth;
    let drawTextY_ = offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.NODE_TEXT_SIZE / 2;
    switch (data.type_) {
      case 1:
      case 2:
      case 3:
      case 4:
        w = pm2f.drawText(NodeTools.jinZhi10ToX(data.value_, data.jinzhi_), MainEditor.NODE_TEXT_SIZE,
          drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_, 1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        break;
      case 5:
        let value = data.value_;
        let keyAndValue = data.parent_.name_ + ' = ' + data.value_;
        if (keyAndValue.length > DISPLAY_TEXT_MAX) {
          value = keyAndValue.substring((data.parent_.name_ + ' = ').length, 27) + '...';
        }
        w = pm2f.drawText('"' + value + '"', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
          1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        break;
      case 6:
        w = this.drawNode(pm2f, this.getNodeText(data), MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY, data.type_, data);
        this.configNodeProc(w, pm2f, data, offx, offy, path);
        if (data.parent_ != undefined) {
          pm2f.drawCut(this.nodeIconCut_, offx + MainEditor.LOGO_LEFT_PADDING - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_),
            offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.LOGO_SIZE / 2);
        }
        break;
      case 7:
        w = this.drawNode(pm2f, data.name_, MainEditor.NODE_TEXT_SIZE, offx, offy + data.posY, data.type_, data);
        this.setNodeButton(pm2f, offx, offy + data.posY, w, MainEditor.NODE_TEXT_SIZE, path, data);
        this.drawObj(pm2f, data.value_, offx + w, offy, path);
        pm2f.drawCut(this.attrIconCut_, offx + MainEditor.LOGO_LEFT_PADDING - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_),
          offy + data.posY + MainEditor.NODE_RECT_HEIGHT / 2 - MainEditor.LOGO_SIZE / 2);
        break;
      case 8:
        this.arrayNodeProc(w, pm2f, data, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), offy);
        break;
      case 9:
        let content = data.parent_.name_ + ' = ';
        if (content.length > DISPLAY_TEXT_MAX) {
          content = '';
        } else if ((content + data.value_).length > DISPLAY_TEXT_MAX) {
          content = data.value_.substring((data.parent_.name_ + ' = ').length, 27) + '...';
        } else {
          content = data.value_;
        }
        w = pm2f.drawText('&' + content, MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
          1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        break;
      case 10:
        w = pm2f.drawText('delete', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
          1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        break;
      case 11:
        if (data.value_) {
          w = pm2f.drawText('true', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
            1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        } else {
          w = pm2f.drawText('false', MainEditor.NODE_TEXT_SIZE, drawTextX_ - (MainEditor.NODE_RECT_WIDTH - data.parent_.parent_.nodeWidth_), drawTextY_,
            1, 1, 0, 1, 1, MainEditor.NODE_TEXT_COLOR);
        }
        break;
      default:
        NapiLog.logError('unknow' + data.type_);
        break;
    }
    if (data.errMsg_ != null) {
      if (parseInt(this.delay_ / 10) % 2 == 0) {
        pm2f.drawRect(offx - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_), offy + data.posY,
          data.nodeWidth_, MainEditor.NODE_RECT_HEIGHT, 0xffff0000, 1);
      }
      pm2f.drawText(data.errMsg_, MainEditor.NODE_TEXT_SIZE, offx - (MainEditor.NODE_RECT_WIDTH - data.parent_.nodeWidth_),
        offy + data.posY + 5, 1, 1, 0, -1, -3, 0xffff0000);
    }
  }

  setNodeButton(pm2f, x, y, w, h, path, node) {
    let rectWidth = MainEditor.NODE_RECT_WIDTH;
    if (node.parent_ == undefined) {
      if (this.nodePoint_ == node) {
        pm2f.drawCut(this.rootIconFocusCut_, x, y);
      } else {
        pm2f.drawCut(this.rootIconCut_, x, y);
      }
      node.nodeWidth_ = MainEditor.NODE_RECT_WIDTH;
      pm2f.drawText(
        node.name_,
        MainEditor.NODE_TEXT_SIZE,
        x + MainEditor.NODE_RECT_WIDTH / 2 - w / 2,
        y + MainEditor.NODE_RECT_HEIGHT / 2 - h / 2,
        1,
        1,
        0,
        1,
        1,
        MainEditor.NODE_TEXT_COLOR
      );
    } else {
      if (node.type_ == DataType.ATTR) {
        let displayValue;
        if (node.value_.type_ == ObjectType.PARSEROP_ARRAY) {
          let arrayValue = NodeTools.arrayToString(node.value_);
          displayValue = '[' + node.value_.value_.length + ']' + arrayValue;
        } else if (
          node.value_.type_ == ObjectType.PARSEROP_UINT8 ||
          node.value_.type_ == ObjectType.PARSEROP_UINT16 ||
          node.value_.type_ == ObjectType.PARSEROP_UINT32 ||
          node.value_.type_ == ObjectType.PARSEROP_UINT64
        ) {
          displayValue = NodeTools.jinZhi10ToX(
            node.value_.value_,
            node.value_.jinzhi_
          );
        } else if (node.value_.type_ == ObjectType.PARSEROP_DELETE) {
          displayValue = 'delete';
        } else if (node.value_.type_ == ObjectType.PARSEROP_BOOL) {
          if (node.value_) {
            displayValue = 'true';
          } else {
            displayValue = 'false';
          }
        } else {
          displayValue = node.value_.value_;
        }

        let keyAndValue;
        let lenDisplay = 27;
        if (node.name_.length <= lenDisplay) {
          keyAndValue = node.name_ + ' = ' + displayValue;
        } else if (node.name_.length == lenDisplay + 1) {
          keyAndValue = node.name_ + ' =';
        } else if (node.name_.length == lenDisplay + 2) {
          keyAndValue = node.name_ + '=';
        } else if (node.name_.length >= DISPLAY_TEXT_MAX) {
          keyAndValue = node.name_;
        }

        if (keyAndValue.length >= DISPLAY_TEXT_MAX) {
          keyAndValue = keyAndValue.substring(0, 27) + '...';
        }
        rectWidth = pm2f.getTextWidth(keyAndValue, MainEditor.NODE_TEXT_SIZE);
      } else {
        rectWidth = pm2f.getTextWidth(
          this.getNodeText(node).length > DISPLAY_TEXT_MAX ? this.getNodeText(node).substring(0, 27) + '...' : this.getNodeText(node),
          MainEditor.NODE_TEXT_SIZE
        );
      }
      this.drawNodeRectButton(pm2f, x, y, rectWidth, node);
    }
    if (this.nodeBtnPoint_ >= this.nodeBtns.length) {
      this.nodeBtns.push(new XButton());
    }
    let pbtn = this.nodeBtns[this.nodeBtnPoint_];
    pbtn.move(
      x - (node.parent_ == undefined ? 0 : MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_),
      y, node.parent_ == undefined ? MainEditor.NODE_RECT_WIDTH : rectWidth + 6 * 2 + MainEditor.LOGO_SIZE + 8, MainEditor.NODE_RECT_HEIGHT
    );
    pbtn.name_ = path;
    pbtn.node_ = node;
    this.nodeBtnPoint_ += 1;
  }

  drawNodeRectButton(pm2f, x, y, rectWidth, node) {
    let width = rectWidth + 6 * 2 + MainEditor.LOGO_SIZE + 8;
    if (node.type_ == DataType.ATTR) {
      switch (node.value_.type_) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 8:
          width = width;
          break;
        default:
          width = width + 14;
          break;
      }
    }
    if (this.nodePoint_ == node) {
      if (this.isSearchResult_) {
        pm2f.drawCut(this.leftSearchAttrCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut(this.centerSearchAttrCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut(this.rightSearchAttrCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      } else {
        pm2f.drawCut(this.leftRectFocusCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut(this.centerFocusCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut(this.rightRectFocusCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      }
    } else {
      if (this.searchKey != null && node.name_.indexOf(this.searchKey) > -1 && this.isSearchResult_) {
        pm2f.drawCut( this.leftSearchFocusCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut(this.centerSearchCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut( this.rightSearchFocusCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      } else {
        pm2f.drawCut( this.leftRectCicleCut_, x - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
        pm2f.drawCut( this.centerRectCut_, x + 8 - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y, width / 116);
        pm2f.drawCut(this.rightRectCicleCut_, x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_), y);
      }
    }
    node.nodeWidth_ = 8 * 2 + width;
    let tx = x + 8 + width - (MainEditor.NODE_RECT_WIDTH - node.parent_.nodeWidth_) + 8;
    if (this.maxX < tx) {
      this.maxX = tx;
    }
  }

  /**
   * 绘制节点数
   * @param {} pm2f X2DFast
   * @param {*} x 起始x坐标
   * @param {*} y 起始y坐标
   * @param {*} h 节点高度
   * @param {*} node 节点
   */
  setNodeMoreButton(pm2f, x, y, w, h, node) {
    if (this.nodeMoreBtnPoint_ >= this.nodeMoreBtns.length) {
      this.nodeMoreBtns.push(new XButton());
    }
    let pbtn = this.nodeMoreBtns[this.nodeMoreBtnPoint_];
    if (node.parent_ == undefined) {
      pbtn.move(x + MainEditor.NODE_RECT_WIDTH + MainEditor.NODE_SIZE_BG_OFFX,
        y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / 2, w, h);
    } else {
      if (node.isOpen_) {
        pbtn.move(x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
          y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / 2, w, h);
      } else {
        pbtn.move(x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
          y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / 2, w, h);
      }
    }
    if (node.isOpen_) {
      pm2f.drawCut(this.circleOpenCut_, x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
        y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / 2);
    } else {
      pm2f.drawCut(this.circleCut_, x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX,
        y + (MainEditor.NODE_RECT_HEIGHT - MainEditor.NODE_MORE_CHILD) / 2);
      let textWidth = pm2f.getTextWidth(node.value_.length, 10);
      pm2f.drawText(node.value_.length, 10, x + node.nodeWidth_ + MainEditor.NODE_SIZE_BG_OFFX + MainEditor.NODE_MORE_CHILD / 2 - textWidth / 2,
        y + MainEditor.NODE_RECT_HEIGHT / 2 - 4, 1, 1, 0, 1, 1, 0xffffffff);
    }
    pbtn.node_ = node;
    this.nodeMoreBtnPoint_ += 1;
  }

  draw(pm2f) {
    const DRAW_HEIGHT = 4;
    getVsCodeTheme();
    pm2f.fillRect(0, 0, Scr.logicw, Scr.logich, MainEditor.CANVAS_BG);
    if (this.filePoint_ != null && this.filePoint_ in this.files_) {
      let data = this.files_[this.filePoint_];
      this.calcPostionY(data, 0);
      if (this.modifyPos_) {
        this.offX_ -= this.modifyPos_.node.posX - this.modifyPos_.x;
        this.offY_ -= this.modifyPos_.node.posY - this.modifyPos_.y;
        this.modifyPos_ = null;
      } else if (this.isFirstDraw) {
        this.offX_ = 0;
        this.offY_ = -data.posY + Scr.logich / 2;
        this.maxX = 0;
        this.drawObj(pm2f, data, this.offX_, this.offY_, '');
        pm2f.fillRect(0, 0, Scr.logicw, Scr.logich, MainEditor.CANVAS_BG);
        this.offX_ = (Scr.logicw - this.maxX) / 2;
        this.isFirstDraw = false;
      }
      this.nodeBtnPoint_ = 0;
      this.nodeMoreBtnPoint_ = 0;
      this.drawObj(pm2f, data, this.offX_, this.offY_, '');
    }
    pm2f.fillRect(0, 0, window.innerWidth, DRAW_HEIGHT, MainEditor.CANVAS_LINE);
    pm2f.fillRect(
      window.innerWidth - 420 - DRAW_HEIGHT,
      0,
      DRAW_HEIGHT,
      window.innerHeight,
      MainEditor.CANVAS_LINE
    );
    pm2f.fillRect(0, 4, window.innerWidth - 420 - 4, 48, MainEditor.CANVAS_BG);
    pm2f.fillRect(
      0,
      52,
      window.innerWidth - 420 - DRAW_HEIGHT,
      DRAW_HEIGHT,
      MainEditor.CANVAS_LINE
    );
    this.sltInclude.setColor(MainEditor.CANVAS_BG, MainEditor.NODE_TEXT_COLOR);
    this.sltInclude.move(16, 20, window.innerWidth - 420 - DRAW_HEIGHT - 16, 20).draw();

    if (this.selectNode_.type != null) {
      if (this.selectNode_.type == 'change_target') {
        pm2f.drawText(
          '点击选择目标',
          14,
          this.mousePos_.x,
          this.mousePos_.y,
          1,
          1,
          0,
          -3,
          -3,
          MainEditor.NODE_TEXT_COLOR
        );
        this.btnCancelSelect_.name_ = '取消选择';
      } else if (this.selectNode_.type == 'copy_node') {
        pm2f.drawText(
          '已复制' + this.selectNode_.pnode.name_,
          14,
          this.mousePos_.x,
          this.mousePos_.y,
          1,
          1,
          0,
          -3,
          -3,
          MainEditor.NODE_TEXT_COLOR
        );
        this.btnCancelSelect_.name_ = '取消复制';
      } else if (this.selectNode_.type == 'cut_node') {
        pm2f.drawText(
          '已剪切' + this.selectNode_.pnode.name_,
          18,
          this.mousePos_.x,
          this.mousePos_.y,
          1,
          1,
          0,
          -3,
          -3,
          MainEditor.NODE_TEXT_COLOR
        );
        this.btnCancelSelect_.name_ = '取消剪切';
      }
      this.btnCancelSelect_.move(Scr.logicw - 250, Scr.logich - 30, 100, 20);
    }

    if (this.errorMsg_.length > 0) {
      let ts = new Date().getTime();
      while (this.errorMsg_.length > 0 && this.errorMsg_[0][0] < ts) {
        this.errorMsg_.shift();
      }
      for (let i in this.errorMsg_) {
        let y = Scr.logich / 2 - this.errorMsg_.length * 20 + i * 20;
        let a = parseInt((this.errorMsg_[i][0] - ts) / 2);
        if (a > MAX_RANDOM) {
          a = MAX_RANDOM;
        }
        NapiLog.logError(a);
        a = a << 24;
        pm2f.fillRect(0, y, Scr.logicw, 20, 0xff0000 | a);
        pm2f.drawText(
          this.errorMsg_[i][1],
          14,
          Scr.logicw / 2,
          y,
          1,
          1,
          0,
          -2,
          -1,
          MainEditor.NODE_TEXT_COLOR
        );
      }
    }
    this.delay_ += 1;
    RightMenu.Draw();
    if (this.searchInput) {
      let x = this.searchInput.pos[0];
      let y = this.searchInput.pos[1];
      let w = this.searchInput.pos[2];
      let h = this.searchInput.pos[3];
      pm2f.drawCut(this.searchBgCut_, x, y);
      pm2f.drawCut(this.searchCut_, x + 28, y + 56 / 2 - 8);
      x = x + 16 + 290 + 16;

      let searchResultTxt =
        this.searchInput.result.length == 0 ? 'No result' : this.searchInput.point + 1 + '/' + this.searchInput.result.length;
      x += pm2f.drawText(
        searchResultTxt,
        MainEditor.NODE_TEXT_SIZE,
        x,
        y + 56 / 2 + 3,
        1,
        1,
        0,
        -1,
        -2,
        0xffffffff
      );
      x += 74 - pm2f.getTextWidth(searchResultTxt, MainEditor.NODE_TEXT_SIZE);
      pm2f.drawCut(this.upCut_, x, y + 56 / 2 - 8);
      this.searchInput.btnUp.move(x, y + 56 / 2 - 8, 16, 16);
      x += 16 + 16;
      pm2f.drawCut(this.downCut_, x, y + 56 / 2 - 8);
      this.searchInput.btnDown.move(x, y + 56 / 2 - 8, 16, 16);
      x += 16 + 16;
      pm2f.drawCut(this.closeCut_, x, y + 56 / 2 - 8);
      this.searchInput.btnClose.move(x, y + 56 / 2 - 8, 16, 16);
    }
    this.procAll();
  }

  buttonClickedProc(nodeBtns) {
    if (
      this.selectNode_.type == null ||
      this.selectNode_.type == 'copy_node' ||
      this.selectNode_.type == 'cut_node'
    ) {
      this.nodePoint_ = nodeBtns.node_;
      AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
      return true;
    }
    if (this.selectNode_.type == 'change_target') {
      let pn = nodeBtns.node_;
      if (pn.type_ == DataType.NODE) {
        if (this.selectNode_.pnode.type_ == DataType.NODE) {
          if (
            NodeTools.getPathByNode(this.selectNode_.pnode.parent_) ==
            NodeTools.getPathByNode(pn.parent_)
          ) {
            this.selectNode_.pnode.ref_ = pn.name_;
          } else {
            this.selectNode_.pnode.ref_ = NodeTools.getPathByNode(pn);
          }
        } else if (this.selectNode_.pnode.type_ == DataType.REFERENCE) {
          if (
            NodeTools.getPathByNode(this.selectNode_.pnode.parent_.parent_) ==
            NodeTools.getPathByNode(pn.parent_)
          ) {
            this.selectNode_.pnode.value_ = pn.name_;
          } else {
            this.selectNode_.pnode.value_ = NodeTools.getPathByNode(pn);
          }
        }

        this.selectNode_.type = null;
        AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
        this.onAttributeChange('writefile');
      } else {
        XMessage.gi().send('WrongNode', '');
      }
    }
    return true;
  }

  dropAllLocked(msg, x, y) {
    if (msg == 2) {
      this.offX_ += x - this.dropAll_.oldx;
      this.offY_ += y - this.dropAll_.oldy;
      this.dropAll_.oldx = x;
      this.dropAll_.oldy = y;
    }
    if (msg == 3) {
      this.dropAll_.locked = false;
    }
  }

  procTouch(msg, x, y) {
    const ADD = 6;
    const DELETE = 7;
    if (this.searchInput) {
      if (XTools.InRect(x, y, ...this.searchInput.pos)) {
        if (this.searchInput.btnUp.procTouch(msg, x, y)) {
          if (this.searchInput.btnUp.isClicked()) {
            this.searchInput.point -= 1;
            if (this.searchInput.point < 0) {
              this.searchInput.point = this.searchInput.result.length - 1;
            }
            this.locateNode(this.searchInput.result[this.searchInput.point]);
          }
        }
        if (this.searchInput.btnDown.procTouch(msg, x, y)) {
          if (this.searchInput.btnDown.isClicked()) {
            this.searchInput.point += 1;
            if (this.searchInput.point >= this.searchInput.result.length) {
              this.searchInput.point = 0;
            }
            this.locateNode(this.searchInput.result[this.searchInput.point]);
          }
        }
        return true;
      } else {
        if (msg == 1) {
          this.searchInput = null;
          this.isSearchResult_ = false;
        }
        return true;
      }
    }

    if (RightMenu.Touch(msg, x, y)) {
      return true;
    }
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
        } else if (nodeBtns.isRightClicked()) {
          this.onAttributeChange('change_current_select', nodeBtns.node_);
          switch (nodeBtns.node_.type_) {
            case ADD:
              RightMenu.Reset(
                [
                  RightMenu.Button(null, 'Add Child Node', null, () => {
                    this.procAddNodeAction();
                    this.onAttributeChange('writefile');
                  }),
                  RightMenu.Button(null, 'Add Sub Property', null, () => {
                    this.procAddAttAction();
                    this.onAttributeChange('writefile');
                  }),
                  RightMenu.Button(null, 'Delete', null, () => {
                    this.procDeleteAction();
                    this.onAttributeChange('writefile');
                  }),
                ],
                nodeBtns.posX_,
                nodeBtns.posY_ + MainEditor.NODE_RECT_HEIGHT
              );
              break;
            case DELETE:
              RightMenu.Reset(
                [
                  RightMenu.Button(null, 'Delete', null, () => {
                    this.procDeleteAction();
                    this.onAttributeChange('writefile');
                  }),
                ],
                nodeBtns.posX_,
                nodeBtns.posY_ + +MainEditor.NODE_RECT_HEIGHT
              );
              break;
          }
        }

        return true;
      }
    }

    for (let i = 0; i < this.nodeMoreBtnPoint_; i++) {
      if (this.nodeMoreBtns[i].procTouch(msg, x, y)) {
        let nodeMoreBtn = this.nodeMoreBtns[i];
        if (nodeMoreBtn.isClicked()) {
          this.buttonClickedProc(nodeMoreBtn);
          this.nodeMoreBtns[i].node_.isOpen_ =
            !this.nodeMoreBtns[i].node_.isOpen_;
          this.modifyPos_ = {
            node: this.nodeMoreBtns[i].node_,
            x: this.nodeMoreBtns[i].node_.posX,
            y: this.nodeMoreBtns[i].node_.posY,
          };
        }
        return true;
      }
    }

    if (msg == 1 && !this.dropAll_.locked) {
      this.dropAll_.locked = true;
      this.dropAll_.oldx = x;
      this.dropAll_.oldy = y;
      return true;
    }
    return false;
  }

  procAddNodeAction() {
    let pattr = AttrEditor.gi();
    pattr.changeDataNodeNotInherit('add_child_node', 'button', '');
  }

  procAddAttAction() {
    let pattr = AttrEditor.gi();
    pattr.changeDataNodeNotInherit('add_child_attr', 'button', '');
  }

  procDeleteAction() {
    let pattr = AttrEditor.gi();
    pattr.changeDataNodeNotInherit('delete', 'button', '');
  }
  searchNodeByName(data, name, out) {
    this.searchKey = name;
    if (data.name_.indexOf(name) >= 0) {
      out.push(data);
    }
    switch (data.type_) {
      case 6:
        for (let i in data.value_) {
          this.searchNodeByName(data.value_[i], name, out);
        }
        break;
      case 7:
        this.searchNodeByName(data.value_, name, out);
        break;
    }
  }
  expandAllParents(curNdoe) {
    if (curNdoe.parent_) {
      curNdoe.parent_.isOpen_ = true;
      this.expandAllParents(curNdoe.parent_);
    }
  }
  locateNode(node) {
    if (!node) {
      return;
    }
    this.expandAllParents(node);
    this.isSearchResult_ = true;
    this.offX_ -= node.posX - Scr.logicw / 2;
    this.offY_ -= this.offY_ + node.posY - Scr.logich / 2;
    this.nodePoint_ = node;
    AttrEditor.gi().freshEditor();
  }
  procKey(k) {
    if (k == 'ctrl+z') {
      if (this.selectNode_.type != null) {
        this.selectNode_.type = null;
      }

      console.log('!!! popHistory ', this.historyZ.length);
      let h;
      if (this.historyZ.length <= 0) {
        h = this.historyBase[this.filePoint_];
      } else {
        if (this.historyZ.length > 1 && this.historyPushed) {
          this.historyZ.pop();
          this.historyPushed = false;
        }
        h = this.historyZ.pop();
      }

      this.filePoint_ = h.fn;
      this.rootPoint_ = h.fn;
      Lexer.FILE_AND_DATA[this.filePoint_] = h.data;
      this.parse(this.filePoint_);
      if (h.sel) {
        this.nodePoint_ = NodeTools.getNodeByPath(
          this.files_[this.filePoint_],
          h.sel
        );
      } else {
        this.nodePoint_ = null;
      }
      AttrEditor.gi().freshEditor(this.filePoint_, this.nodePoint_);
    } else if (k == 'ctrl+f') {
      this.searchInput = {
        pos: [(Scr.logicw - 300) / 2, Scr.logich / 4, 450, 40],
        result: [],
        point: 0,
        btnUp: new XButton(0, 0, 0, 0, '上一个'),
        btnDown: new XButton(0, 0, 0, 0, '下一个'),
        btnClose: new XButton(0, 0, 0, 0, '关闭'),
      };
      let x = this.searchInput.pos[0];
      let y = this.searchInput.pos[1];
      let w = this.searchInput.pos[2];
      let h = this.searchInput.pos[3];
      CanvasInput.Reset(x, y + (h - 20) / 2, 258, 32, '', null, (v) => {
        this.searchInput.result = [];
        if (v.length > 0) {
          this.searchNodeByName(
            this.files_[this.filePoint_],
            v,
            this.searchInput.result
          );
          if (this.searchInput.result.length > 0) {
            this.locateNode(this.searchInput.result[0]);
            this.searchInput.point = 0;
          }
        }
      });
      CanvasInput.SetSafeArea(...this.searchInput.pos);
    } else if (k == 'ctrl+c') {
      if (this.nodePoint_ != null) {
        this.selectNode_ = {
          type: 'copy_node',
          pnode: this.nodePoint_,
        };
      }
    } else if (k == 'ctrl+x') {
      if (this.nodePoint_ != null) {
        this.selectNode_ = {
          type: 'cut_node',
          pnode: this.nodePoint_,
        };
      }
    } else if (k == 'ctrl+v') {
      if (this.selectNode_.type != null && this.nodePoint_ != null) {
        let parent = this.nodePoint_;
        if (this.nodePoint_.type_ != DataType.NODE) {
          parent = this.nodePoint_.parent_;
        }
        parent.value_.push(NodeTools.copyNode(this.selectNode_.pnode, parent));
        if (this.selectNode_.type == 'cut_node') {
          ModifyNode.deleteNode(this.selectNode_.pnode);
          this.selectNode_.type = null;
        }
        this.checkAllError();
      }
    } else if (k == 'Delete') {
      if (this.nodePoint_ != null) {
        ModifyNode.deleteNode(this.nodePoint_);
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
    MainEditor.gi().rootPoint_ = sel;
    AttrEditor.gi().freshEditor();
  }

  nodeCount(data) {
    let ret = 1;
    switch (data.type_) {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        break;
      case 6:
        for (let i in data.value_) {
          ret += this.nodeCount(data.value_[i]);
        }
        break;
      case 7:
        ret += this.nodeCount(data.value_);
        break;
      case 8:
      case 9:
      case 10:
      case 11:
        break;
      default:
        NapiLog.logError('unknow' + data.type_);
        break;
    }
    return ret;
  }
  isNodeCountChanged(fn, bset = true) {
    if (!(fn in this.nodeCount_)) {
      this.nodeCount_[fn] = -1;
    }
    let newcount = this.nodeCount(this.files_[fn]);
    if (this.nodeCount_[fn] != newcount) {
      if (bset) {
        this.nodeCount_[fn] = newcount;
      }
      return true;
    }
    return false;
  }
  saveHistory(fn, data2, pth, pos = null) {
    console.log('!!! save History ', this.historyZ.length, pos);
    if (fn in this.historyBase) {
      this.historyZ.push({
        fn: fn,
        data: data2,
        sel: pth,
      });
      this.historyPushed = true;
    } else {
      this.historyBase[fn] = {
        fn: fn,
        data: data2,
        sel: pth,
      };
    }
  }
  onAttributeChange(type, value) {
    let pme = MainEditor.gi();
    if (type == 'writefile') {
      let data1 = Generator.gi().makeHcs(pme.filePoint_, pme.files_[pme.filePoint_]);
      let data2 = [];
      for (let j in data1) {
        data2.push(data1.charCodeAt(j));
      }
      if (pme.isNodeCountChanged(pme.filePoint_)) {
        Lexer.FILE_AND_DATA[pme.filePoint_] = data2;
        pme.parse(pme.filePoint_);
        let t = NodeTools.getPathByNode(pme.nodePoint_, false);
        if (t) {
          pme.nodePoint_ = NodeTools.getNodeByPath(pme.files_[pme.filePoint_], t);
        } else {
          pme.nodePoint_ = null;
        }
        if (pme.selectNode_.pnode) {
          let t = NodeTools.getPathByNode(pme.selectNode_.pnode, false);
          if (t) {
            pme.selectNode_.pnode = NodeTools.getNodeByPath(pme.files_[pme.filePoint_], t);
          } else {
            pme.selectNode_.pnode = null;
          }
        }
        AttrEditor.gi().freshEditor(pme.filePoint_, pme.nodePoint_);
      }
      pme.checkAllError();
      XMessage.gi().send('writefile', {
        fn: pme.filePoint_,
        data: data1,
      });
      pme.saveHistory(pme.filePoint_, data2, NodeTools.getPathByNode(pme.nodePoint_), 1);
    } else if (type.substring(0, 13) == 'change_target') {
      pme.selectNode_.type = type;
      pme.selectNode_.pnode = value;
    } else if (type.startsWith('cancel_change_target')) {
      pme.selectNode_.type = null;
    } else if (type == 'change_current_select') {
      pme.nodePoint_ = value;
      AttrEditor.gi().freshEditor(pme.filePoint_, pme.nodePoint_);
    }
  }
  onError(msg) {}
  onTouch(msg, x, y) {
    this.touchQueue_.push([msg, x, y]);
  }
  onKey(k) {
    this.keyQueue_.push(k);
  }
  onReceive(type, data) {
    console.log('onReceive type=' + type);
    NapiLog.logError(type);
    let me = MainEditor.gi();
    if (type == 'parse') {
      me.parse(data);
    } else if (type == 'filedata') {
      me.saveHistory(data.fn, data.data, null, 2);
      Lexer.FILE_AND_DATA[data.fn] = data.data;
      me.parse(data.fn);
    } else if (type == 'freshfiledata') {
      me.saveHistory(data.fn, data.data, null, 3);
      Lexer.FILE_AND_DATA[data.fn] = data.data;
    } else if (type == 'whiteCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initCutData(wurl);
    } else if (type == 'circleImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initCicleImgData(wurl);
    } else if (type == 'cicleOpenImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initcicleOpenImgData(wurl);
    } else if (type == 'rectangleFocusImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initRectangleFocusImgData(wurl);
    } else if (type == 'nodeIconImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initNodeIconImgData(wurl);
    } else if (type == 'attrIconImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initAttrIconImgData(wurl);
    } else if (type == 'rootIconImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initRootIconImgData(wurl);
    } else if (type == 'rootIconFocusImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initRootIconFocusImgData(wurl);
    } else if (type == 'backgroundImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initBackgroundImgData(wurl);
    } else if (type == 'popItemFocusImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initPopItemFocusImgData(wurl);
    } else if (type == 'colorThemeChanged') {
      me.reloadMenuBgPic();
    } else if (type == 'searchBgImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchBgImgData(wurl);
    } else if (type == 'upCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initUpImgData(wurl);
    } else if (type == 'downCut') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initDownImgData(wurl);
    } else if (type == 'closeCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initCloseImgData(wurl);
    } else if (type == 'searchCutImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchImgData(wurl);
    } else if (type == 'searchNoodRectImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchNoodRectImgData(wurl);
    } else if (type == 'searchAttrRectImg') {
      let u8arr = new Uint8Array(data.data);
      let imgobj = new Blob([u8arr], { type: 'image/png' });
      let wurl = window.URL.createObjectURL(imgobj);
      me.initSearchAttrRectImgData(wurl);
    } else {
      NapiLog.logError('onReceive is not need');
    }
  }

  initSearchAttrRectImgData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const CS_X_VAL = 8;
    const RS_X_VAL = 124;
    const CS_OLD_WIDTH = 116;
    this.searchAttrCicleImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.leftSearchAttrCicleCut_ = XTexture.gi().makeCut(this.searchAttrCicleImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerSearchAttrCut_ = XTexture.gi().makeCut(this.searchAttrCicleImg_, CS_X_VAL, 0, CS_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightSearchAttrCicleCut_ = XTexture.gi().makeCut(this.searchAttrCicleImg_, RS_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  initSearchNoodRectImgData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const CS_X_VAL = 8;
    const RS_X_VAL = 124;
    const CS_OLD_WIDTH = 116;
    this.searchRectFocusCicleImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.leftSearchFocusCicleCut_ = XTexture.gi().makeCut(this.searchRectFocusCicleImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerSearchCut_ = XTexture.gi().makeCut(this.searchRectFocusCicleImg_, CS_X_VAL, 0, CS_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightSearchFocusCicleCut_ = XTexture.gi().makeCut(this.searchRectFocusCicleImg_, RS_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  initSearchImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.searchImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.searchCut_ = XTexture.gi().makeCut(this.searchImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initCloseImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.closeImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.closeCut_ = XTexture.gi().makeCut(this.closeImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initDownImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.downImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.downCut_ = XTexture.gi().makeCut(this.downImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initUpImgData(wurl) {
    const WIDTH = 16;
    const HEIGHT = 16;
    this.upImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.upCut_ = XTexture.gi().makeCut(this.upImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initSearchBgImgData(wurl) {
    const WIDTH = 494;
    const HEIGHT = 56;
    this.searchBgImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.searchBgCut_ = XTexture.gi().makeCut(this.searchBgImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initPopItemFocusImgData(wurl) {
    const WIDTH = 148;
    const HEIGHT = 32;
    RightMenu.popItemFocusImg_ = XTexture.gi().loadTextureFromImage(wurl);
    RightMenu.popItemFocusCut_ = XTexture.gi().makeCut(RightMenu.popItemFocusImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initBackgroundImgData(wurl) {
    const WIDTH = 156;
    const HEIGHT = 112;
    RightMenu.backgroundImg_ = XTexture.gi().loadTextureFromImage(wurl);
    RightMenu.backgroundCut_ = XTexture.gi().makeCut(RightMenu.backgroundImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initRootIconFocusImgData(wurl) {
    const WIDTH = 132;
    const HEIGHT = 32;
    this.rootIconFocusImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.rootIconFocusCut_ = XTexture.gi().makeCut(this.rootIconFocusImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initRootIconImgData(wurl) {
    const WIDTH = 132;
    const HEIGHT = 32;
    this.rootIconImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.rootIconCut_ = XTexture.gi().makeCut(this.rootIconImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initAttrIconImgData(wurl) {
    const WIDTH = 8;
    const HEIGHT = 8;
    this.attrIconImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.attrIconCut_ = XTexture.gi().makeCut(this.attrIconImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initNodeIconImgData(wurl) {
    const WIDTH = 8;
    const HEIGHT = 8;
    this.nodeIconImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.nodeIconCut_ = XTexture.gi().makeCut(this.nodeIconImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initcicleOpenImgData(wurl) {
    const WIDTH = 20;
    const HEIGHT = 20;
    this.cicleOpenImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.circleOpenCut_ = XTexture.gi().makeCut(this.cicleOpenImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initRectangleFocusImgData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const RF_OLD_WIDTH = 132;
    const CF_X_VAL = 8;
    const CF_OLD_WIDTH = 116;
    const RR_X_VAL = 124;
    this.rectangleFocusImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.rectangleFocusCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, 0, 0, RF_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.leftRectFocusCicleCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerFocusCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, CF_X_VAL, 0, CF_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightRectFocusCicleCut_ = XTexture.gi().makeCut(this.rectangleFocusImg_, RR_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  initCicleImgData(wurl) {
    const WIDTH = 20;
    const HEIGHT = 20;
    this.cicleImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.circleCut_ = XTexture.gi().makeCut(this.cicleImg_, 0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT);
  }

  initCutData(wurl) {
    const HEIGHT = 32;
    const NEW_WIDTH = 132;
    const OLD_WIDTH = 8;
    const CR_X_VAL = 8;
    const RR_X_VAL = 124;
    const WC_OLD_WIDTH = 132;
    const CR_OLD_WIDTH = 116;
    this.whiteImg_ = XTexture.gi().loadTextureFromImage(wurl);
    this.whiteCut_ = XTexture.gi().makeCut(this.whiteImg_, 0, 0, WC_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.leftRectCicleCut_ = XTexture.gi().makeCut(this.whiteImg_, 0, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.centerRectCut_ = XTexture.gi().makeCut(this.whiteImg_, CR_X_VAL, 0, CR_OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
    this.rightRectCicleCut_ = XTexture.gi().makeCut(this.whiteImg_, RR_X_VAL, 0, OLD_WIDTH, HEIGHT, NEW_WIDTH, HEIGHT);
  }

  syncOpenStatus(newNode, oldParentNode) {
    let oldNode = null;
    for (let i = 0; i < oldParentNode.value_.length; ++i) {
      if (newNode.name_ === oldParentNode.value_[i].name_) {
        oldNode = oldParentNode.value_[i];
      }
    }
    if (oldNode == null) {
      return;
    }
    newNode.isOpen_ = oldNode.isOpen_;

    for (let j = 0; j < newNode.value_.length; ++j) {
      this.syncOpenStatus(newNode.value_[j], oldNode);
    }
  }

  syncRootStatus(newRoot, oldRoot) {
    newRoot.isOpen_ = oldRoot.isOpen_;
    for (let i = 0; i < newRoot.value_.length; ++i) {
      this.syncOpenStatus(newRoot.value_[i], oldRoot);
    }
  }

  parse(fn) {
    if (this.rootPoint_ == null) {
      this.rootPoint_ = fn;
    }
    let t = Generator.gi().hcsToAst(fn);
    if (!t) {
      return;
    }

    let fs = [];
    for (let i in t) {
      let newRoot = Generator.gi().astToObj(t[i].ast.astRoot_);

      if (this.files_[i]) {
        this.syncRootStatus(newRoot, this.files_[i]);
      }

      this.files_[i] = newRoot;
      fs.push(i);
    }
    this.filePoint_ = this.rootPoint_;
    this.sltInclude.resetList(fs, this.filePoint_);
    AttrEditor.gi().setFiles(this.files_);

    this.checkAllError();
  }

  checkAllError() {
    NapiLog.clearError();
    let n1 = Generator.gi().mergeObj(this.files_);
    if (n1) {
      n1 = Generator.gi().expandObj(n1);
      if (NapiLog.getResult()[0]) {
        return true;
      }
    }
    return false;
  }
}
MainEditor.LINE_HEIGHT = 50;
MainEditor.NODE_RECT_HEIGHT = 32;
MainEditor.NODE_RECT_WIDTH = 132;
MainEditor.NODE_TEXT_COLOR = 0xffffffff;
MainEditor.NODE_TEXT_SIZE = 14;
MainEditor.BTN_CONTENT_OFFY = 4;
MainEditor.NODE_TEXT_OFFX = 5;
MainEditor.NODE_LINE_COLOR = 0xff979797;
MainEditor.NODE_SIZE_BG_OFFX = 4;
MainEditor.NODE_MORE_CHILD = 20;
MainEditor.LINE_WIDTH = 30;
MainEditor.LOGO_LEFT_PADDING = 14;
MainEditor.LOGO_SIZE = 8;

MainEditor.pInstance_ = null;
MainEditor.gi = function () {
  if (MainEditor.pInstance_ == null) {
    MainEditor.pInstance_ = new MainEditor();
  }
  return MainEditor.pInstance_;
};

module.exports = {
  MainEditor,
};
