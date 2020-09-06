-- Base file taken from: https://github.com/Stubatiger/can-wireshark-dissector
-- constants
local sid_dict_description = {
    [0x10] = "Start Diagnostic Session",
    [0x20] = "Stop Diagnostic Session",
    [0x3E] = "Tester Present",
    [0x27] = "Security Access",
    [0x11] = "ECU Reset",
    [0x81] = "Start Comm",
    [0x82] = "Stop Comm",
    [0x28] = "Disable Normal Message Transm.",
    [0x29] = "Enable Normal Message Transm.",
    [0x85] = "Control DTC Setting",
    [0x83] = "Access Timing Parameters",
    [0x84] = "Network Configuration",

    [0x21] = "Read Data by Local Identifier",
    [0x22] = "Read Data by Common Identifier",
    [0x1A] = "Read ECU Identification",
    [0x23] = "Read Memory by Address",
    [0x3B] = "Write Data by Local Identifier",
    [0x2E] = "Write Data by Common Identifier",
    [0x3d] = "Write Memory by Address",
    [0x26] = "Set Data Rates",
    [0x2c] = "Dynamically define Data Identifier",

    [0x14] = "Clear Diagnostic Information",
    [0x13] = "Read Diagnostic Trouble Code",
    [0x12] = "Read Freeze Frame Data",
    [0x18] = "Read DTCs by Status",
    [0x17] = "Read Status of DTCs",

    [0x30] = "Input Output Control by Local ID",
    [0x2F] = "Input Output Control by Common ID",

    [0x31] = "Start Routine by Local Identifier",
    [0x32] = "Stop Routine by Local Identifier",
    [0x38] = "Start Routine by Address",
    [0x39] = "Stop Routine by Address",
    [0x33] = "Request Routine Results by Local ID",
    [0x3A] = "Request Routine Results by Address",

    [0x34] = "Request Download",
    [0x35] = "Request Upload",
    [0x36] = "Transfer Data",
    [0x37] = "Request Transfer Exit",

    [0x7F] = "Negative Response",
    [0x3F] = "Negative Response" -- Masked Negative Response
}

local response_codes = {
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
    [0x78] = "Busy - Response pending",
    [0x7E] = "Service or Subfunction not supported",
    [0x7F] = "Service or Subfunction not supported",
}

local positive_response_mask = 0x40
local negative_response = 0x7F

-- fields
local length = ProtoField.new("Payload Length", "kwp2k.length", ftypes.UINT8,nil, base.DEC)
local sid = ProtoField.uint8("kwp2k.sid", "Service ID", base.HEX, sid_dict_description)
local sid_copy = ProtoField.uint8("kwp2k.sidcopy", "Service ID Copy", base.HEX, sid_dict_description)
local pid = ProtoField.new("Parameter Identifier", "kwp2k.pid", ftypes.BYTES)
local response = ProtoField.uint8("kwp2k.response","Response", base.HEX, sid_dict_description, 0xBF)
local response_data = ProtoField.new("Reponse Data", "kwp2k.rdata", ftypes.BYTES)
local response_code = ProtoField.uint8("kwp2k.rcode", "Response Code", base.HEX, response_codes)

-- declare dissector
local my_kwp_2000_dissector = Proto.new("kwp2k", "KWP2000")

my_kwp_2000_dissector.fields = {
	length,
    sid,
    sid_copy,
    pid,
    response_code,
    response_data,
    response,
}

local partialBuffer = nil
pktState = {}

function my_kwp_2000_dissector.dissector(tvbuf,pktinfo,root)
    -- set the protocol column to show our protocol name
    pktinfo.cols.protocol:set("KWP2000")
    local pktlen = tvbuf:reported_length_remaining()
    local tree = root:add(my_kwp_2000_dissector, tvbuf:range(0,pktlen))

    -- We can select predefined colors from here, or setup colors in coloring scheme using same conditions
    --pink 1
	--set_color_filter_slot(1, "kwp2k && kwp2k.sidcopy") --negative responses

    --pink 2
    --purple 1
    --purple 2
    --green 1
    --green 2

    --green 3
    --set_color_filter_slot(7, "kwp2k && kwp2k.response && !kwp2k.sidcopy") --positive response

    -- yellow 1
    --set_color_filter_slot(8, "kwp2k && !kwp2k.response && !kwp2k.sidcopy && kwp2k.length") --requests

    --yellow 2
    --gray
	
    tree:add(length, tvbuf:len())

    local cur_sid = tvbuf:range(0,1):uint()
    pktlen = tvbuf:len()

    if cur_sid == negative_response then    -- negative response
        tree:add(response, tvbuf:range(0,1))
        tree:add(sid_copy, tvbuf:range(1,1))
        tree:add(response_code, tvbuf:range(2,1))
        pktinfo.cols.info = "NOK"
    elseif bit32.btest(cur_sid, positive_response_mask) then -- positive response (bit 6 is set)
        tree:add(response, tvbuf:range(0,1))
        tree:add(pid, tvbuf:range(1,pktlen-1))
        pktinfo.cols.info = "OK"
    elseif sid_dict_description[cur_sid] ~= nil then    -- normal request
        tree:add(sid, tvbuf:range(0,1))
        tree:add(pid, tvbuf:range(1,pktlen-1))
		pktinfo.cols.info = sid_dict_description[cur_sid]
    end
end

--Asign to protocol 0x93: VWTP2.0
local ipProtocol = DissectorTable.get("ip.proto")
ipProtocol:add(0x93, my_kwp_2000_dissector)
