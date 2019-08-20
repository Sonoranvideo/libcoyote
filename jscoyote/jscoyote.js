var ref = require('ref');
var ffi = require('ffi');
var Struct = require('ref-struct');

var TimeCodeStruct = Struct({'ScrubBar' : 'double',
							'Time' : 'int32',
							'TRT' : 'int32',
							'PresetKey' : 'int32',
							'Selected' : 'bool'});

var TimeCodeStructPtr = ref.refType(TimeCodeStruct);

							
var Decls = ffi.Library('./libcoyote',
		{
			'CoyoteSession_New' : ['pointer', ['string'] ],
			'CoyoteSession_Destroy' : ['void', ['pointer'] ],
			'Coyote_Take' : [ 'int', ['pointer', 'int32'] ],
			'Coyote_End' : [ 'int', ['pointer', 'int32'] ],
			'Coyote_Pause' : [ 'int', ['pointer', 'int32'] ],
			'Coyote_TakeNext' : [ 'int', ['pointer' ] ],
			'Coyote_TakePrev' : [ 'int', ['pointer' ] ],
			'Coyote_SelectNext' : [ 'int', ['pointer' ] ],
			'Coyote_SelectPrev' : [ 'int', ['pointer' ] ],
			'Coyote_RebootCoyote' : [ 'int', ['pointer' ] ],
			'Coyote_SoftRebootCoyote' : [ 'int', ['pointer' ] ],
			'Coyote_ShutdownCoyote' : [ 'int', ['pointer' ] ],
			'Coyote_RestartService' : [ 'int', ['pointer' ] ],
			'Coyote_EjectDisk' : [ 'int', ['pointer', 'string' ] ],
			'Coyote_SelectPreset' : [ 'int', ['pointer', 'int32'] ],
			'Coyote_DeletePreset' : [ 'int', ['pointer', 'int32'] ],
			'Coyote_SetHardwareMode' : [ 'int', ['pointer', 'int', 'int'] ],
			'Coyote_InitializeCoyote' : [ 'int', ['pointer', 'int', 'int'] ],
			'Coyote_SeekTo' : [ 'int', ['pointer', 'int32', 'uint32'] ],
			'Coyote_GetTimeCode' : [ 'int', ['pointer', 'int32', TimeCodeStructPtr ] ]
		});

Object.assign(exports, Decls, { "TimeCodeStructPtr" : TimeCodeStructPtr, "TimeCodeStruct" : TimeCodeStruct });
