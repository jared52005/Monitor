-- LUA bitwise operations: https://www.lua.org/manual/5.2/manual.html#6.7
-- constants
local sid_dict_description = {
    [0x01] = "Show current data",
    [0x02] = "Show freeze frame data",
    [0x03] = "Show stored Diagnostic Trouble Codes",
    [0x04] = "Clear DTC (Emission related ECUs)",
    [0x05] = "Test Results, Oxygen sensor monitoring (non CAN only)",
    [0x06] = "Test results, other component/system monitoring (CAN only)",
    [0x07] = "Show pending Diagnostic Trouble Codes",
    [0x08] = "Control operation of on-board component/system",
    [0x09] = "Request vehicle information",
    [0x0A] = "Permanent Diagnostic Trouble Codes (DTCs) (Cleared DTCs)",
    [0x10] = "Start Diagnostic Session",
    [0x11] = "ECU Reset",
    [0x12] = "Read Freeze Frame Data",
    [0x13] = "Read Diagnostic Trouble Code",
    [0x14] = "Clear DTC (Standard method)",
    [0x19] = "Read DTC information",
    [0x17] = "Read Status of DTCs",
    [0x18] = "Read DTCs by Status",
    [0x22] = "RDBLI",
    [0x23] = "RMBA",
    [0x2E] = "WDBLI",
    [0x26] = "Set Data Rates",
    [0x27] = "Security Access",
    [0x28] = "Disable Normal Message Transm.",
    [0x29] = "Enable Normal Message Transm.",
    [0x2c] = "Dynamically define Data Identifier",
    [0x2F] = "Input Output Control by Common ID",
    [0x30] = "Input Output Control by Local ID",
    [0x31] = "Routine Control",
    [0x34] = "Request Download",
    [0x35] = "Request Upload",
    [0x36] = "Transfer Data",
    [0x37] = "Request Transfer Exit",
    [0x3d] = "WMBA",
    [0x3E] = "Tester Present",
    [0x83] = "Access Timing Parameters",
    [0x84] = "Network Configuration",
    [0x85] = "Control DTC Setting",

    [0x7F] = "", --Negative Response
}

local nrc_description = {
    [0x10] = "General reject",
    [0x11] = "Service or Subfunction not supported",
    [0x12] = "Service or Subfunction not supported",
    [0x13] = "Message length or format incorrect",
    [0x21] = "Busy - Repeat request",
    [0x22] = "Conditions not correct",
    [0x24] = "Request sentence error",
    [0x31] = "Out of range",
    [0x33] = "Security access denied",
    [0x35] = "Invalid key",
    [0x36] = "Exceed attempts",
    [0x72] = "General Programming Failure",
    [0x78] = "Busy - Response pending",
    [0x7E] = "Service or Subfunction not supported",
    [0x7F] = "Service or Subfunction not supported",
}

function NegativeResponse_InfoColumn(tvbuf, pktinfo)
    -- `[7F2233] - RDBLI; Security Requested` = Service ID (22 = RDBLI) Negative Response code (33 = Security)
    local preview = tostring(tvbuf:range(0,3))
    local nrc = nrc_description[tvbuf:range(2,1):uint()]
    local sid = tvbuf:range(1,1):uint()
    if nrc == nil then
        nrc= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[sid] .. "; " .. nrc
end

-- UDS 04
function EDTC_PID_InfoColumn(tvbuf, pktinfo)
    -- `[04] - Erase DTC`
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x04]
end

local reqVehInfo_PID_dict_description = {
    ["02"] = "VIN",
    ["06"] = "CVN Maybe",
}

-- UDS 09
function ReqVehInfo_PID_InfoColumn(tvbuf, pktinfo)
    -- `[09] - Request Vehicle Information`
    local preview = tostring(tvbuf:range(0,1))
    local type = reqVehInfo_PID_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x09] .. "; " .. type
end

-- UDS 10
local sds_dict_description = {
    ["01"] = "Default",
    ["02"] = "Programming",
    ["03"] = "Extended",
}
function SDS_InfoColumn(tvbuf, pktinfo)
    -- `[1001] - Start Diagnostic Session; Default`
    local preview = tostring(tvbuf:range(0,2))
    local type = sds_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x10] .. "; " .. type
end

-- UDS 11
local erst_dict_description = {
    ["01"] = "Hard reset",
}
function ERST_InfoColumn(tvbuf, pktinfo)
    -- `[1101] - Start Diagnostic Session; Default`
    local preview = tostring(tvbuf:range(0,2))
    local type = erst_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x10] .. "; " .. type
end

-- UDS 14
function EDTC_InfoColumn(tvbuf, pktinfo)
    -- `[14] - Erase DTC`
    local preview = tostring(tvbuf:range(0,4))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x14]
end

-- UDS 19
function RDTC_InfoColumn(tvbuf, pktinfo)
    -- `[19] - Read DTC`
    local preview = tostring(tvbuf:range(0,3))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x19]
end

--UDS 22
local rdbli_dict_description = {
    ["02e0"] = "WFS Challenge",
    ["02ed"] = "WFS Status",
    ["02ee"] = "WFS Status Slaves",
    ["02ef"] = "WFS Locktimes",
    ["02f9"] = "WFS CS (501+)",
    ["02ff"] = "WFS Snapshot",
    ["ef90"] = "WFS Status SHE",
    ["0405"] = "State of flash memory (VAG)",
    ["0407"] = "Amount of programming attempts (VAG)", -- How many times Erase was called as [00 01] [00 01] [00 01] = 1/1/1 counters
    ["0408"] = "Amount of successful attempts (VAG)", -- How many times we get through Check Memory as [00 01] [00 01] [00 01] = 1/1/1 counters
    ["0600"] = "Coding (VAG)",
    ["f15b"] = "Fingerprint (VAG)",
    ["f17c"] = "FAZIT (VAG)", --ID and Serial number string printed on label of the ECU
    ["f186"] = "Active Diagnostic Session",
    ["f187"] = "Software Number",
    ["f189"] = "Software Revision",
    ["f18c"] = "ECU Serial Number (VAG)",
    ["f190"] = "VIN",
    ["f191"] = "Hardware Number",
    ["f197"] = "ECU Type",
    ["f19e"] = "ASAM name (VAG)",
    ["f1a2"] = "ASAM version (VAG)",
    ["f1a5"] = "Coding or serial number",
    ["f1a3"] = "Hardware Revision VAG", --Hnn if stock hardware. Xnn if TD1 / TG1 is triggered
    ["f1aa"] = "VAG System Name", --How this component is called in diagrams / electrical schematics
    ["f1ad"] = "Engine Code Letters (VAG)",
    ["f1df"] = "Programming Information", --Usually means if ECU is in application (0x40) or in a bootloader (0x44). Could be also related to TD1 / TG1
    ["f1f4"] = "Bootloader ID (VAG)",
    ["f442"] = "ECU Voltage (VAG)",
    ["f806"] = "CVN (VAG)", --You should be able to get CVN via 0x0A PID 
}

function RDBLI_InfoColumn(tvbuf, pktinfo)
    -- `[22F190] - RDBLI; VIN` = First 3 bytes of request, Service ID (22 = RDBLI) Subfunciton ID (F190 = VIN)
    local preview = tostring(tvbuf:range(0,3))
    local type = rdbli_dict_description[tostring(tvbuf:range(1,2))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x22] .. "; " .. type
end

--UDS 23
function RMBA_InfoColumn(tvbuf, pktinfo)
    -- `[23 14 A001420C 10] - RDBLI; VIN` = First 3 bytes of request, Service ID (22 = RDBLI) Subfunciton ID (F190 = VIN)
    local preview = tostring(tvbuf:range(0,1))

    if preview == "23" then
        local fmt = tvbuf:range(1,1):uint()
        local fmt_length = bit32.rshift(bit32.band(fmt, 0xF0), 4)
        local fmt_address = bit32.band(fmt, 0xF)

        local address = tostring(tvbuf:range(2, fmt_address))
        local length = tostring(tvbuf:range(2 + fmt_address,fmt_length))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x23] .. "; " .. string.upper(address) .. " [" .. string.upper(length) .. "]"
    else
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x23]
    end

end

--UDS 27
function SA_InfoColumn(tvbuf, pktinfo)
    -- `[2701] - Seceurity access`
    local preview = tostring(tvbuf:range(0,2))
    local type = "Level: " .. tostring(tvbuf:range(1,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x27] .. "; " .. type
end

--UDS 28
function COMCtrl_InfoColumn(tvbuf, pktinfo)
    -- `[28xx] - Communication Control`
    local preview = tostring(tvbuf:range(0,2))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x28]
end

--UDS 2E
local wdbli_dict_description = {
    ["02e1"] = "WFS Login; 4",
    ["02e2"] = "WFS 5 Download; 52",
    ["02f6"] = "WFS 5 Download (SHE); 176",
    ["02fb"] = "WFS 4 Download; 52",
    ["f15a"] = "Progamming Fingerprints",
    ["f198"] = "Repair Shop Code",
}

function WDBLI_InfoColumn(tvbuf, pktinfo)
    -- `[2EF190] - RDBLI; VIN` = First 3 bytes of request, Service ID (22 = RDBLI) Subfunciton ID (F190 = VIN)
    local preview = tostring(tvbuf:range(0,3))
    local type = wdbli_dict_description[tostring(tvbuf:range(1,2))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x2E] .. "; " .. type
end

--UDS 31
local rc_dict_description = {
    ["0202"] = "Block Checksum",
    ["0203"] = "Programming Preconditions",
    ["ff00"] = "Erase",
    ["ff01"] = "Check programming dependencies",
}
function RC_InfoColumn(tvbuf, pktinfo)
    -- `[31010203] - Tester Present; Default`
    local preview = tostring(tvbuf:range(0,2))
    local type = rc_dict_description[tostring(tvbuf:range(2,2))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x31] .. "; " .. type
end

--UDS 34
function RD_InfoColumn(tvbuf, pktinfo)
    sid = tvbuf:range(0,1):uint()
    if(sid == 0x34 ) then
        -- `[34 1e c0 00 01 01 3e 00] - Request download <address> [<length>]`
        local preview = tostring(tvbuf:range(0,1))

        local fmt = tvbuf:range(2,1):uint()
        local fmt_length = bit32.rshift(bit32.band(fmt, 0xF0), 4)
        local fmt_address = bit32.band(fmt, 0xF)

        local address = tostring(tvbuf:range(3, fmt_address))
        local length = tostring(tvbuf:range(3 + fmt_address,fmt_length))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x34] .. "; " .. string.upper(address) .. " [" .. string.upper(length) .. "]"
    else
        local preview = tostring(tvbuf:range(0,1))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x34]
    end
end

--UDS 35
function RU_InfoColumn(tvbuf, pktinfo)
    sid = tvbuf:range(0,1):uint()
    if(sid == 0x35 ) then
        -- `[35 1e c0 00 01 01 3e 00] - Request Upload <address> [<length>]`
        local preview = tostring(tvbuf:range(0,1))

        local fmt = tvbuf:range(2,1):uint()
        local fmt_length = bit32.rshift(bit32.band(fmt, 0xF0), 4)
        local fmt_address = bit32.band(fmt, 0xF)

        local address = tostring(tvbuf:range(3, fmt_address))
        local length = tostring(tvbuf:range(3 + fmt_address,fmt_length))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x35] .. "; " .. string.upper(address) .. " [" .. string.upper(length) .. "]"
    else
        local preview = tostring(tvbuf:range(0,1))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x35]
    end
end

--UDS 36
function T_InfoColumn(tvbuf, pktinfo)
    -- `[36] - Data transfer
    local preview = tostring(tvbuf:range(0,1))
    local seq = tostring(tvbuf:range(1,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "...] - " ..  sid_dict_description[0x36] .. "; SEQ = " .. seq
end

--KWP 37
function RTE_InfoColumn(tvbuf, pktinfo)
    -- `[37] - Request transfer exit
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x37]
end

--UDS 3E
local tp_dict_description = {
    ["00"] = "Default",
    ["80"] = "Supres response",
}

function TP_InfoColumn(tvbuf, pktinfo)
    -- `[3E00] - Tester Present; Default`
    local preview = tostring(tvbuf:range(0,2))
    local type = tp_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x3E] .. "; " .. type
end

--UDS 85
local dtcctrl_dict_description = {
    ["00"] = "Default",
}

function DTCCtrl_InfoColumn(tvbuf, pktinfo)
    -- `[85xx] - Control DTC Settings`
    local preview = tostring(tvbuf:range(0,2))
    local type = dtcctrl_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x85] .. "; " .. type
end

local sid_dict_methods = {
    [0x04] = EDTC_PID_InfoColumn,
    [0x09] = ReqVehInfo_PID_InfoColumn,
    [0x10] = SDS_InfoColumn,
    [0x11] = ERST_InfoColumn,
    [0x14] = EDTC_InfoColumn,
    [0x19] = RDTC_InfoColumn,
    [0x22] = RDBLI_InfoColumn,
    [0x23] = RMBA_InfoColumn,
    [0x27] = SA_InfoColumn,
    [0x28] = COMCtrl_InfoColumn,
    [0x2E] = WDBLI_InfoColumn,
    [0x31] = RC_InfoColumn,
    [0x34] = RD_InfoColumn,
	[0x35] = RU_InfoColumn,
    [0x36] = T_InfoColumn,
    [0x37] = RTE_InfoColumn,
    [0x3E] = TP_InfoColumn,
    [0x85] = DTCCtrl_InfoColumn,
}

local positive_response_mask = 0x40
local negative_response = 0x7F

-- fields
local length = ProtoField.new("Payload Length", "uds_vag.length", ftypes.UINT8,nil, base.DEC)
local sid = ProtoField.uint8("uds_vag.sid", "Service ID", base.HEX, sid_dict_description)
local sid_ispos = ProtoField.uint8("uds_vag.is_positive", "Pos. Res; Service ID", base.HEX, sid_dict_description)
local sid_isneg = ProtoField.uint8("uds_vag.is_negative", "Negative Response", base.HEX, sid_dict_description)
local data = ProtoField.new("Data", "uds_vag.data", ftypes.BYTES)
local nrc = ProtoField.uint8("uds_vag.nrc", "Negative Response Code", base.HEX, nrc_description)

-- declare dissector
local uds_dissector = Proto.new("uds_vag", "UDS (VAG)")

uds_dissector.fields = {
	length,
    sid,
    sid_ispos,
    sid_isneg,
    nrc,
    data,
}

function uds_dissector.dissector(tvbuf,pktinfo,root)
    -- set the protocol column to show our protocol name
    pktinfo.cols.protocol:set("UDS")
    local pktlen = tvbuf:reported_length_remaining()
    local tree = root:add(uds_dissector, tvbuf:range(0,pktlen))
    tree:add(length, tvbuf:len())

    local cur_sid = tvbuf:range(0,1):uint()
    pktlen = tvbuf:len()

    --Is negative response?
    if cur_sid == negative_response then
        tree:add(sid_isneg, tvbuf:range(0,1))
        tree:add(sid, tvbuf:range(1,1))
        tree:add(nrc, tvbuf:range(2,1))
        NegativeResponse_InfoColumn(tvbuf, pktinfo)
    --Is positive response? (SID | 0x40)
    elseif bit32.btest(cur_sid, positive_response_mask) then
        tree:add(sid_ispos, tvbuf:range(0,1))
        tree:add(data, tvbuf:range(0,pktlen))
        if sid_dict_methods[cur_sid - 0x40] ~= nul then
            sid_dict_methods[cur_sid - 0x40](tvbuf, pktinfo)
        end
    --Is request?
    elseif sid_dict_description[cur_sid] ~= nil then
        tree:add(sid, tvbuf:range(0,1))
        tree:add(data, tvbuf:range(0,pktlen))
        if sid_dict_methods[cur_sid] ~= nul then
            sid_dict_methods[cur_sid](tvbuf, pktinfo)
        end
    end
end

--Asign to protocol 0x94: ISO15765
local ipProtocol = DissectorTable.get("ip.proto")
ipProtocol:add(0x94, uds_dissector)
