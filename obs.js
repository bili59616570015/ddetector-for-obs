// ==UserScript==
function waitForElement(node, interval = 10, maxAttempts = 1000) {
    let attempts = 0;
    let timerId;

    function checkElement() {
        node = document.querySelector(
            "[data-e2e=\"quality-selector\"]"
        );
        if (attempts >= maxAttempts) {
            console.error("Reached maximum number of attempts. Element not found.");
            clearTimeout(timerId);
            return;
        }

        if (node === undefined || node === null) {
            console.error("Element not found");
            attempts++;
            timerId = setTimeout(checkElement, interval);
        } else {
            console.log("Element found");
            clearTimeout(timerId);
        }
    }

    checkElement();
}

function selectQuality() {
    const qualitySelectorDiv = document.querySelector(
        "[data-e2e=\"quality-selector\"]"
    );
    waitForElement(qualitySelectorDiv);
    if (qualitySelectorDiv) {
        const firstChildElement = qualitySelectorDiv.firstElementChild;

        if (firstChildElement) {
            const clickEvent = new MouseEvent("click", {
                bubbles: true,
                cancelable: true,
                view: window
            });

            firstChildElement.dispatchEvent(clickEvent);
        }
    }
}

function checkStatePeriodically() {
    setInterval(() => {
        const element = document.querySelector(".xgplayer-play");
        const loadingElement = document.querySelector(".xgplayer-loading");
        var needRefresh = false;
        if (element) {
            const newState = element.getAttribute("data-state");
            console.log("Checking state:", newState);
            if (newState === "pause") {
                needRefresh = true;
            }
        }
        if (loadingElement) {
            const computedStyle = window.getComputedStyle(loadingElement);
            const displayValue = computedStyle.getPropertyValue("display");
            
            if (displayValue == "block") {
                needRefresh = true;
            }
        }
        if (needRefresh) {
            needRefresh = false;
            console.log("Paused. Pressing E...");
            pressKey("e");
        }
    }, 40000); // Check every 40,000 milliseconds (40 seconds)
}

// 
function pressKey(key) {
    console.log(`模拟按下 ${key} 键...`);
    console.log(`Key${key.toUpperCase()}`);
    document.body.focus();
    document.dispatchEvent(new KeyboardEvent("keydown", { key: key, code: `Key${key.toUpperCase()}`, bubbles: true }));
    document.dispatchEvent(new KeyboardEvent("keypress", { key: key, code: `Key${key.toUpperCase()}`, bubbles: true }));
    document.dispatchEvent(new KeyboardEvent("keyup", { key: key, code: `Key${key.toUpperCase()}`, bubbles: true }));
}

function setupCss() {
    var obsCSS = document.createElement("style");
    // obsCSS.textContent = ".webcast-chatroom___content-with-emoji-text { font-size: 19px !important; line-height: 1.5; }";
    obsCSS.textContent = ".webcast-chatroom * { font-size: 19px !important; line-height: 1.5em !important; }; .webcast-chatroom___item-wrapper div { padding: 2px 0 !important; } ";
    document.querySelector("head").appendChild(obsCSS);
}

function moveMouse() {
    setInterval(() => {
        const element = document.body;

        const mouseMoveEvent = new MouseEvent("mousemove", {
            bubbles: true,
            clientX: 100, // x coordinate
            clientY: 100  // y coordinate
        });

        element.dispatchEvent(mouseMoveEvent);
    }, 5000); // Move mouse every second
}

function wait(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

checkStatePeriodically();
moveMouse();

async function start() {
    await wait(1000);
    document.body.style.zoom = "150%";
	setupCss();
    selectQuality();
	await wait(1000);
    pressKey("e");
    await wait(1000);
    pressKey("y"); // fullscreen
    pressKey("b");
}

start();