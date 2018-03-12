// Copyright (c) 2017-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

//TBD: 1. Load Button strings as localized strings
//     2. Support for checkATSC30LegacyBoxSupport => Working
//     3. Button functionality => Working
//     4. Key event handling => working
//     5. MouseOver/Mouse Move event handling
//     6. Exit App/Settings Button are working but Network Menu not launched. => Working

err_info = JSON.parse(JSON.stringify(error_info));

var isRTL = (err_info.textdirection === "rtl") ? true : false ;

//Default reference font sizes wrt full HD (1920 x 1080)
var errortitle_fontSize = 126; // Error title
var errordetails_fontSize = 30; // Error details
var errorinfo_fontSize = 30; // Error info
// Buttons
var buttons_height = 85;
var buttons_lineHeight = 75;
var buttons_borderRadius = 9999;
var buttons_fontSize = 33;

// Size unit for styles
var size_unit = "px";

var ButtonEnum = {
  SETTINGS_BUTTON : 1
};

var TargetEnum = {
  NETWORK : 1,
  GENERAL : 2
};

/* Key code list for key event */
var leftkey = 37;
var rightkey = 39;
var enterkey = 13;
/* Array for buttons on page */
var g_buttons = new Array;
var g_numButtons = 0; // Number of buttons
var lastfocus = -1;   // Button focus position
var cursor_x = 0; // X-position of cursor

/* Handle webOSRelaunch event */
function relaunchHandler(e) {
  function onclosecallback() {
    var param = {};
    var q_param = {};
    var serviceCall_url = "luna://com.webos.applicationManager/launch";
    param["id"] = PalmSystem.identifier;
    q_param["query"] = e.detail.query;
    param["params"] = q_param;
    PalmSystem.serviceCall(serviceCall_url, param);
  }
  PalmSystem.onclose = onclosecallback;
  window.close();
}
document.addEventListener('webOSRelaunch', relaunchHandler);

/* Check if it's ATSC 3.0 STB */
function checkATSC30LegacyBoxSupport() {
  return new Promise( function(resolve) {
    var retry = 0;
    var isSupport;
    var isConfigUpdated = false;
    var interval = setInterval(
      function() {
        retry += 1;
        if (retry > 5) {
          clearInterval(interval);
          console.log("Retry Count: " + retry + " [X]");
          resolve(false); // send false as a default as timeout
        }
        var url = "luna://com.webos.service.config/getConfigs";
        var config = {"configNames":["tv.hw.atsc30legacybox"]};
        var param = JSON.stringify(config);
        var response;
        var palmObject = new PalmServiceBridge();
        palmObject.onservicecallback = function(msg) {
          response = JSON.parse(msg);
          isSupport = response.configs['tv.hw.atsc30legacybox'] === "ATSC30_LEGACYBOX_SUPPORT";
          if (isSupport !== undefined)
            isConfigUpdated = true;
        };

        palmObject.call(url, param);

        if (isConfigUpdated === true) {
          clearInterval(interval);
          resolve(isSupport); // send config-compared result
        }
      }, 10);
  });
}

window.onmousemove = function(e) {
  cursor_x = e.clientX;
  if (cursor_x < 0)
    cursor_x = 0;
}

function cursorStateHandler(e) {
  var visibility = e.detail.visibility;
  if (!visibility && lastfocus === -1){
    // Find nearest button from last cursor position
    var near_btn = 0;
    var near_btn_pos = 0.0;
    var button_pos;
    var rect;
    for (var b = 0; b < g_numButtons; b++) {
      rect = g_buttons[b][0].getBoundingClientRect();
      button_pos = (rect.right + rect.left) * 0.5;
      if (b === 0) {
        near_btn_pos = button_pos;
        continue;
      }
      if ( Math.abs(button_pos - cursor_x) < Math.abs(near_btn_pos - cursor_x) ) {
        near_btn = b;
        near_btn_pos = button_pos;
      }
    }
    // Set focus on default button
    setOnFocus(near_btn);
  }
}
document.addEventListener('cursorStateChange', cursorStateHandler);

window.onkeyup = function checkEnter(e) {
  var key = e.which || e.keyCode || 0;
  if (key === enterkey) {
    if (lastfocus == -1) {  // Set focus on default button if there's no focus
      var btn_no = isRTL ? g_numButtons-1 : 0;
      setOnFocus(btn_no);
    }
    else
      g_buttons[lastfocus][1]();  // Execute function of entered button
  }
}

window.onkeydown = function checkMoveFocus(e) {
  var key = e.which || e.keyCode || 0;

  // Detect left or right keys only
  if(key != leftkey && key != rightkey) return;

  // If there is no focused button, focus on first button
  if(lastfocus == -1) {
    setOnFocus(0);
    return;
  }

  // Ignore input key if next button is not exist
  if(((key==leftkey) && (lastfocus-1 < 0)) ||
      (key==rightkey) && (lastfocus+1 > g_numButtons-1))
    return;

  // Move focus to next buttn
  if(key === leftkey)
    lastfocus -= 1;
  else
    lastfocus += 1;

  setOnFocus(lastfocus);
}

function onMouseOverBtn() {
  for(var i = 0; i < g_numButtons; i++) {
    if(this.id === g_buttons[i][0].id)
      setOnFocus(i);  // Focus on this button
    else if(g_buttons[i][0].classList.contains("spotlight"))
      setOutFocus(i); // Focus out from other buttons with spotlight
  }
}

function onMouseOutBtn() {
  this.classList.remove("spotlight");
  this.blur();
  lastfocus = -1;
}

var onExitApp = function() {
  window.close();
}

function setOnFocus(no) {
  // Ignore invalid button index and already focused button
  if(no < 0 || no > g_numButtons-1) return;
  if(g_buttons[no][0].classList.contains("spotlight"))
    return;

  // Set spotlight and focus on the button
  g_buttons[no][0].classList.add("spotlight");
  g_buttons[no][0].focus();
  lastfocus = no;

  // Make only one button be with spotlight
  for(var i = 0; i<g_numButtons; i++) {
    if((g_buttons[i][0].classList.contains("spotlight")) && (i != no)) {
      g_buttons[i][0].classList.remove("spotlight");
      g_buttons[i][0].blur();
    }
  }
}

function setOutFocus(no) {
  // Ignore invalid button index and already not focused button
  if(no < 0 || no > g_numButtons-1) return;
  if(!g_buttons[no][0].classList.contains("spotlight"))
    return;
  // Remove spotlight and focus from the button
  g_buttons[no][0].classList.remove("spotlight");
  g_buttons[no][0].blur();
  lastfocus = -1;
}

window.onkeydown = function checkMoveFocus(e) {
  var key = e.which || e.keyCode || 0;

  // Detect left or right keys only
  if(key != leftkey && key != rightkey) return;

  // If there is no focused button, focus on first button
  if(lastfocus == -1) {
    setOnFocus(0);
    return;
  }

  // Ignore input key if next button is not exist
  if(((key==leftkey) && (lastfocus-1 < 0)) ||
      (key==rightkey) && (lastfocus+1 > g_numButtons-1))
    return;

  // Move focus to next buttn
  if(key === leftkey)
    lastfocus -= 1;
  else
    lastfocus += 1;

  setOnFocus(lastfocus);
}


function getCurrentTime() {
  var now = new Date();
  var currentTime = "";

  function setDateFormat(d) {
    // Set date format as double digits
    var result = "";
    if(d < 10)
      result = "0" + d;
    else
      result = d;
    return result;
  }

  currentTime = now.getFullYear() + "-" + setDateFormat(now.getMonth()+1) + "-" + setDateFormat(now.getDate());
  currentTime = currentTime + "\u0009" + now.toTimeString();

  return currentTime + "\u200e";
}

function getErrorInfo(error) {
  switch (error) {
    //case -105:
    //case -137:
    //  return hostname;
    case -201:
      return document.getElementById("errorinfo").innerHTML  + " : " + getCurrentTime();
    default:
      return "";
  }
}

function getLayoutCase(error) {
  switch (error) {
    case -109:    // ADDRESS_UNREACHABLE
      return 0;   // : Exit App button
    case -105:    // NAME_NOT_RESOLVED
    case -106:    // INTERNET_DISCONNECTED
    case -137:    // NAME_RESOLUTION_FAILED
      return 2;   // : Exit App, Retry, Network settings
    case -201:    // CERT_DATE_INVALID
      return 3;     // EXIT APP, RETRY, SETTINGS
    default:
      return 1;   // : Exit App, Retry
  }
}

function createButtonOnPage(str_name, str_id, b_dir, func) {
  var btn = document.createElement("BUTTON");
  var btnString = document.createTextNode(str_name);
  var need_small_btn = window.matchMedia( "(max-width: 1280px)" ).matches;

  // Set button properties
  btn.appendChild(btnString);
  btn.id = str_id;
  btn.className = "enyo-tool-decorator moon-large-button-text moon-button moon-composite min-width";
  if(need_small_btn) {
    // Need small button for low resolution
    btn.classList.add("small");
    btn.classList.add("moon-small-button-text");
  }
  btn.tabIndex = 0
  btn.type = "button";
  btn.addEventListener("click", func);
  btn.addEventListener("mouseover", onMouseOverBtn);
  btn.addEventListener("mouseout", onMouseOutBtn);
  btn.setAttribute("role", "button");

  // Set button position and append to parent element
  var btnContainer = (b_dir === "Button_Left") ? "Button_Left" : "Button_Right";
  document.getElementById(btnContainer).appendChild(btn);

  // Add button on list
  g_buttons[g_numButtons] = new Array;
  g_buttons[g_numButtons][0] = document.getElementById(str_id);
  g_buttons[g_numButtons][1] = func;
  g_numButtons += 1;

}

var onRetryApp = function () {
  window.location = err_info.url_to_reload;
}

function setButtonsOnPage(layoutCase, isRTL) {
  var dir_head = isRTL ? "Button_Right" : "Button_Left";
  var dir_tail = isRTL ? "Button_Left" : "Button_Right";
  //<div class="moon-application-content-container">
  //<div id="Button_Container" class="button-container">
  //<div id="Button_Left" class="button-position-left"><button i18n-content="exitappbutton" id="ExitApp_Button" class="enyo-tool-decorator moon-large-button-text moon-button moon-composite min-width spotlight" tabindex="0" type="button" role="button">EXIT APP</button></div>
  //<div id="Button_Right" class="button-position-right"><button i18n-content="networksettingbutton" id="NetworkSetting_Button" class="enyo-tool-decorator moon-large-button-text moon-button moon-composite min-width" tabindex="0" type="button" role="button">NETWORK SETTINGS</button>
  //<button i18n-content="retry_button" id="Retry_Button" class="enyo-tool-decorator moon-large-button-text moon-button moon-composite min-width" tabindex="0" type="button" role="button">RETRY</button></div></div>

  switch(layoutCase) {
    case 0: // Exit
      createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
      break;
    case 1: // Exit, Retry
      if(isRTL) {
        createButtonOnPage(err_info.retry_button_text, "Retry_Button", dir_tail, onRetryApp);
        createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
      }
      else {
        createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
        createButtonOnPage(err_info.retry_button_text, "Retry_Button", dir_tail, onRetryApp);
      }
      break;
    case 2: // Exit, Network setting, Retry buttons
      if(isRTL) {
        createButtonOnPage(err_info.retry_button_text, "Retry_Button", dir_tail, onRetryApp);
        createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
      }
      else {
        createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
        createButtonOnPage(err_info.retry_button_text, "Retry_Button", dir_tail, onRetryApp);
      }
      break;
    case 3: // Exit, Settings, Retry buttons
      if(isRTL) {
        createButtonOnPage(err_info.retry_button_text, "Retry_Button", dir_tail, onRetryApp);
        createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
      }
      else {
        createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
        createButtonOnPage(err_info.retry_button_text, "Retry_Button", dir_tail, onRetryApp);
      }
      break;

    default: // Exit
      createButtonOnPage(err_info.exit_app_button_text, "ExitApp_Button", dir_head, onExitApp);
      break;
  }
}

function initFocus() {
  // To prevent timing issue that button is ignored by alert elemets,
  // set alert role on header before set focus on button
  var header = document.getElementById("error_header");
  header.setAttribute("role", "alert");

  // Set 'Exit App' button by default.
  if(isRTL){
    g_buttons[g_numButtons-1][0].blur();
    setOnFocus(g_numButtons-1);
  }
  else {
    g_buttons[0][0].blur();
    setOnFocus(0);
  }
}

function setButtonStyle() {
  console.log("Application Viewport: [" + appResolutionWidth +", " + appResolutionHeight +"]");

  // We set page style only if resolution is other than 1920 x 1080
  if (appResolutionWidth == 0 || appResolutionHeight == 0){
    appResolutionWidth = screen.width;
    appResolutionHeight = screen.height;
  }

  var buttons = document.getElementsByTagName("BUTTON");

  // Buttons
  var i;
  for (i = 0; i < buttons.length; i++) {
    buttons[i].style.height = ((appResolutionWidth * buttons_height) / (screen.width)) + size_unit;
    buttons[i].style.lineHeight = ((appResolutionWidth * buttons_lineHeight) / (screen.width)) + size_unit;
    buttons[i].style.borderRadius = ((appResolutionWidth * buttons_borderRadius) / (screen.width)) + size_unit;
    buttons[i].style.fontSize =  ((appResolutionWidth * buttons_fontSize) / (screen.width)) + size_unit;
  }
}

function setPageStyle() {
  // We set page style only if resolution is other than 1920 x 1080
  if (appResolutionWidth == 0 || appResolutionHeight == 0){
    appResolutionWidth = screen.width;
    appResolutionHeight = screen.height;
  }

  var errortitle = document.getElementById("errortitle");
  var errordetails = document.getElementById("errordetails");
  var errorguide = document.getElementById("errorguide");
  var errorinfo = document.getElementById("errorinfo");

  // Error title
  errortitle.style.fontSize = ((appResolutionWidth * errortitle_fontSize) / (screen.width)) + size_unit;

  // Error details
  errordetails.style.fontSize = ((appResolutionWidth * errordetails_fontSize) / (screen.width)) + size_unit;

  // Error info
  errorinfo.style.fontSize = ((appResolutionWidth * errorinfo_fontSize) / (screen.width)) + size_unit;

  // Error guide
  errorguide.style.fontSize = ((appResolutionWidth * errorinfo_fontSize) / (screen.width)) + size_unit;
}

function setErrorPageMessages(error_title, error_details, error_guide, error_info) {
  //Set Error Information to error Page
  setPageStyle();
  document.getElementById("errortitle").innerHTML = error_title;
  document.getElementById("errordetails").innerHTML = error_details;
  document.getElementById("errorguide").innerHTML = error_guide;
  document.getElementById("errorinfo").innerHTML = error_info;
}

function onload(){
  if (isRTL) {
          document.getElementById("errortitle").style.direction = "rtl";
          document.getElementById("errordetails").style.direction = "rtl";
          document.getElementById("errorguide").style.direction = "rtl";
          document.getElementById("errorinfo").style.direction = "rtl";
          document.getElementById("errorinfo").style.cssFloat = "right";
  }

  //Set Error Information to error Page
  //TBD: Check this again after localization changes
  setErrorPageMessages(err_info.error_title + "\u200e(\u200e" + errorCode + "\u200e)\u200e",
                       err_info.error_details, err_info.error_guide, err_info.error_info);

  setButtonsOnPage(getLayoutCase(errorCode), isRTL);
  setButtonStyle(); // Set button style
  // Set focus on first buttons
  // Checking Audio Guidance is on/off
  if (!document.accessibilityEnabled)
    initFocus();
  else {
    // checking webengine is ready to handle focus event
    if (document.accessibilityReady)
       initFocus();
    else
      document.addEventListener('webOSAccessibilityReady', initFocus, false);
  }

  if (errorCode == -201) {
  // If it's certificate date error(-201),
  // check it's ATSC 3.0 STB first so that show different message
      checkATSC30LegacyBoxSupport().then( function(v) {
        isATSC30LegacyBoxSupport = v;
        if(isATSC30LegacyBoxSupport) {
           setErrorPageMessages(err_info.error_title_atsc_legacy_box + "\u200e(\u200e" + errorCode + "\u200e)\u200e",
                                err_info.error_details_atsc_legacy_box, err_info.error_guide_atsc_legacy_box, err_info.error_info_atsc_legacy_box + " : " + getCurrentTime());
        }
      });
      document.getElementById("errorinfo").innerHTML += " : " + getCurrentTime();
  }
}
