/*
 * Script version 1.5
*/

/** Converts date to ISO string
 * @param {Date} date * @returns {string} */
 function dateToISOString(date) {
    return String(date.getFullYear()).padStart(4, '0') + String(date.getMonth() + 1).padStart(2, '0') +
    String(date.getDate()).padStart(2, '0') + 'T' + String(date.getHours()).padStart(2, '0') + 
    String(date.getMinutes()).padStart(2, '0') + String(date.getSeconds()).padStart(2, '0') + 'Z'
}

/** Converts ISO string to date
 * @param {string} input * @returns {Date | null} */
 function ISOStringToDate(input) {
    const date = new Date(Date.parse(input.replace(
        /(\d\d\d\d)(\d\d)(\d\d)T(\d\d)(\d\d)(\d\d)Z/, "$1-$2-$3T$4:$5:$6")))
    return isNaN(date) ? null : date
}

/** Gets and sets actual current device datetime
 * @param {Date | undefined} deviceDateOrUndefined * @returns {Date | null} */
 function getOrSetDeviceDate(deviceDateOrUndefined) {
    if (deviceDateOrUndefined instanceof Date) {
        deviceTimeOrigin = Date.now()
        deviceTime = deviceDateOrUndefined.getTime()
        return deviceDateOrUndefined
    }

    return typeof deviceTimeOrigin != 'undefined' && typeof deviceTime != 'undefined'
        ? new Date(Date.now() - deviceTimeOrigin + deviceTime)
        : null
}

/** Get or set date from/to date upload control
 * @param {Date | undefined} dateToSetOrUndefined * @returns {Date | undefined} */
function getOrSetUploadDate(dateToSetOrUndefined) {
    const ctrl = document.getElementById('dateinput')
    if (dateToSetOrUndefined instanceof Date) {
        ctrl.value = dateToISOString(dateToSetOrUndefined)
        return dateToSetOrUndefined
    }
    return ISOStringToDate(ctrl.value)
}

/** Get or set timezone
 * @param {number | undefined} timezoneOrUndefined * @returns {number | NaN} */
 function getOrSetTimezone(timezoneOrUndefined) {
    const ctrl = document.getElementById('edittimezone')
    if (typeof timezoneOrUndefined == 'number') {
        ctrl.value = timezoneOrUndefined
        return timezoneOrUndefined
    }
    return parseInt(ctrl.value)
}

/** Get or set daylight
 * @param {number | undefined} daylightOrUndefined * @returns {number | NaN} */
 function getOrSetDaylight(daylightOrUndefined) {
    const ctrl = document.getElementById('editdaylight')
    if (typeof daylightOrUndefined == 'number') {
        ctrl.value = daylightOrUndefined
        return daylightOrUndefined
    }
    return parseInt(ctrl.value)
}

/** Get or set ntpenabled
 * @param {boolean | undefined} ntpEnabledOrUndefined * @returns {boolean} */
 function getOrSetNTPEnabled(ntpEnabledOrUndefined) {
    const ctrl = document.getElementById('checkntpenabled')
    if (typeof ntpEnabledOrUndefined == 'boolean') {
        ctrl.checked = ntpEnabledOrUndefined
        return ntpEnabledOrUndefined
    }
    return ctrl.checked
}

/** Get or set timeservers uri
 * @param {number} index * @param {string | undefined} timeserverUriOrUndefined * @returns {string} */
 function getOrSetTimeserver(index, timeserverUriOrUndefined) {
    const ctrl = document.getElementById('edittimeserver' + index)
    if (typeof timeserverUriOrUndefined == 'string') {
        ctrl.value = timeserverUriOrUndefined
        return timeserverUriOrUndefined
    }
    return ctrl.value
}

/** Get or set display brightness
 * @param {number | undefined} brightnessOrUndefined * @returns {number | NaN} */
 function getOrSetDisplayBrightness(brightnessOrUndefined) {
    const ctrl = document.getElementById('editbrightness')
    if (typeof brightnessOrUndefined == 'number') {
        ctrl.value = brightnessOrUndefined
        return brightnessOrUndefined
    }
    return parseInt(ctrl.value)
}

/** Gets and sets display colors
 * @param {string | undefined} displayColorsOrUndefined * @returns {string} */
function getOrSetDisplayColors(displayColorsOrUndefined) {
    const ctrl = document.getElementById('editcolors')
    if (typeof displayColorsOrUndefined == 'string') {
        ctrl.value = displayColorsOrUndefined
        return displayColorsOrUndefined
    }
    return ctrl.value
}

/** Sets color picker values
 * @param {string} colors */
function updateColorPickers(colors) {
    for(let i = 0; i < 5; ++i) {
        const ctrl = document.getElementById('color' + i)
        ctrl.value = '#' + colors.substring(i * 6, i * 6 + 6)
    }
}

function updateControls(state) {
    getOrSetUploadDate(getOrSetDeviceDate(ISOStringToDate(state.date)))
    getOrSetTimezone(state.timezone)
    getOrSetDaylight(state.daylight)
    getOrSetNTPEnabled(state.ntpenabled)
    getOrSetTimeserver(1, state.ntpserver1)
    getOrSetTimeserver(2, state.ntpserver2)
    getOrSetTimeserver(3, state.ntpserver3)
    getOrSetDisplayBrightness(state.brightness)
    getOrSetDisplayColors(state.colors)
    updateColorPickers(state.colors)
}

function setStatus(text) {
    document.getElementById("statuspanel").innerHTML = 'Status: ' + text
}

function requestState() {
    if (window.location.hostname == '') {
        setStatus("Device state accepted")
        updateControls(JSON.parse('{"date":"20221108T102641Z", "timezone":3, "daylight":0, "ntpenabled":true, "ntpserver1":"0.pool.ntp.org", "ntpserver2":"1.pool.ntp.org", "ntpserver3":"time.nist.gov", "brightness":25, "colors":"0808220000443333AAFF0000001100"}'))
        return
    }

    let rq = new XMLHttpRequest()
    rq.open('GET', 'status', true)
    rq.onreadystatechange = function() {
        if (rq.readyState === 4) {
            console.log(rq.response)            
            updateControls(resp = JSON.parse(rq.response))
            setStatus("Device state accepted")
        }
    }
    rq.send('')
    console.log("GET-status request sent")
}

function updateCurrentTime() {
    const deviceDate = getOrSetDeviceDate()
    if (deviceDate) {
        document.getElementById('datepanel').innerText = deviceDate.toDateString()
        document.getElementById('clockpanel').innerText = deviceDate.toTimeString().substring(0, 8)
    }
}

function buttonPickSystemDateClick() {
    getOrSetUploadDate(new Date(Date.now()))
}

function buttonSetDeviceTimeClick() {
    const date = getOrSetUploadDate()
    let rq = new XMLHttpRequest()
    rq.open('POST', 'set-date', true)
    rq.setRequestHeader("Content-Type", "application/x-www-form-urlencoded")
    rq.onreadystatechange = function() {
        if (rq.readyState === 4) {
            getOrSetDeviceDate(date)
            setStatus("Clock date set: " +  dateToISOString(date))
        }
    }
    rq.send('date=' + dateToISOString(date))
}

function buttonSyncronizeTimeClick() {
    let rq = new XMLHttpRequest()
    rq.open('POST', 'syncronize', true)
    rq.onreadystatechange = function() {
        if (rq.readyState === 4) {
            setStatus("Clock syncronization initiated")
            requestState()
        }
    }
    rq.send('')
}

function buttonCommitDisplaySettingsClick() {
    const brightness = getOrSetDisplayBrightness()
    const colors = getOrSetDisplayColors()

    let rq = new XMLHttpRequest()
    rq.open('POST', 'set-display', true)
    rq.setRequestHeader("Content-Type", "application/x-www-form-urlencoded")
    rq.onreadystatechange = function() {
        if (rq.readyState === 4) {
            setStatus('Display settings commited')
        }
    }
    rq.send(`brightness=${brightness}&colors=${colors}`)
}

function displayColorsTextChanged(value) {
    updateColorPickers(value)
}

function displayColorChaged(index, color) {
    let colors = getOrSetDisplayColors();

    getOrSetDisplayColors(colors.substring(0, index * 6) +
       color.substring(1) + colors.substring(6 + index * 6));
}

document.addEventListener("DOMContentLoaded", function() {
    requestState()
})

setInterval(updateCurrentTime, 1000) // update device time clock



