// Copyright 2019 Daniel Butum

#include "NYSteamResult.h"


FString FNYSteamResultHelper::ResultToString(EResult Result)
{
	// https://partner.steamgames.com/doc/api/steam_api#EResult
	FString ReturnVal;

	#define SteamResultCase(Value, Desc) \
		case Value: ReturnVal = FString::Printf(TEXT("'%i' %s (%s)"), static_cast<int32>(Value), TEXT(#Value), Desc); break;

	switch (Result)
	{
	SteamResultCase(k_EResultOK,								TEXT("success"));
	SteamResultCase(k_EResultFail,								TEXT("failure"));
	SteamResultCase(k_EResultNoConnection,						TEXT("no connection"));
	SteamResultCase(k_EResultInvalidPassword,					TEXT("invalid password/ticket"));
	SteamResultCase(k_EResultLoggedInElsewhere,					TEXT("same user logged in elsewhere"));
	SteamResultCase(k_EResultInvalidProtocolVer,				TEXT("incorrect protocol version"));
	SteamResultCase(k_EResultInvalidParam,						TEXT("a parameter is incorrect"));
	SteamResultCase(k_EResultFileNotFound,						TEXT("file not found"));
	SteamResultCase(k_EResultBusy,								TEXT("called method busy, no action taken"));
	SteamResultCase(k_EResultInvalidState,						TEXT("called object in invalid state"));
	SteamResultCase(k_EResultInvalidName,						TEXT("invalid name"));
	SteamResultCase(k_EResultInvalidEmail,						TEXT("invalid email"));
	SteamResultCase(k_EResultDuplicateName,						TEXT("duplicate name"));
	SteamResultCase(k_EResultAccessDenied,						TEXT("access denied"));
	SteamResultCase(k_EResultTimeout,							TEXT("operation timed out"));
	SteamResultCase(k_EResultBanned,							TEXT("VAC banned"));
	SteamResultCase(k_EResultAccountNotFound,					TEXT("account not found"));
	SteamResultCase(k_EResultInvalidSteamID,					TEXT("steamid invalid"));
	SteamResultCase(k_EResultServiceUnavailable,				TEXT("requested service currently unavailable"));
	SteamResultCase(k_EResultNotLoggedOn,						TEXT("user is not logged on"));
	SteamResultCase(k_EResultPending,							TEXT("request is pending - may be in process, or waiting on third party"));
	SteamResultCase(k_EResultEncryptionFailure,					TEXT("encryption or decryption failed"));
	SteamResultCase(k_EResultInsufficientPrivilege,				TEXT("insufficient privilege"));
	SteamResultCase(k_EResultLimitExceeded,						TEXT("limit exceeded"));
	SteamResultCase(k_EResultRevoked,							TEXT("access revoked"));
	SteamResultCase(k_EResultExpired,							TEXT("license or guest pass expired"));
	SteamResultCase(k_EResultAlreadyRedeemed,					TEXT("guest pass already redeemed"));
	SteamResultCase(k_EResultDuplicateRequest,					TEXT("duplicate request, already occurred, ignoring"));
	SteamResultCase(k_EResultAlreadyOwned,						TEXT("already owned"));
	SteamResultCase(k_EResultIPNotFound,						TEXT("IP address not found"));
	SteamResultCase(k_EResultPersistFailed,						TEXT("failed to write change to data store"));
	SteamResultCase(k_EResultLockingFailed,						TEXT("failed to acquire access lock for operation"));
	SteamResultCase(k_EResultLogonSessionReplaced,				TEXT("???"));
	SteamResultCase(k_EResultConnectFailed,						TEXT("???"));
	SteamResultCase(k_EResultHandshakeFailed,					TEXT("???"));
	SteamResultCase(k_EResultIOFailure,							TEXT("input/output failure"));
	SteamResultCase(k_EResultRemoteDisconnect,					TEXT("???"));
	SteamResultCase(k_EResultShoppingCartNotFound,				TEXT("failed to find shopping cart requested"));
	SteamResultCase(k_EResultBlocked,							TEXT("blocked"));
	SteamResultCase(k_EResultIgnored,							TEXT("ignored"));
	SteamResultCase(k_EResultNoMatch,							TEXT("nothing matching request found"));
	SteamResultCase(k_EResultAccountDisabled,					TEXT("???"));
	SteamResultCase(k_EResultServiceReadOnly,					TEXT("service not accepting content changes right now"));
	SteamResultCase(k_EResultAccountNotFeatured,				TEXT("???"));
	SteamResultCase(k_EResultAdministratorOK,					TEXT("allowed to take this action, but only because requester is admin"));
	SteamResultCase(k_EResultContentVersion,					TEXT("version mismatch in transmitted content"));
	SteamResultCase(k_EResultTryAnotherCM,						TEXT("???"));
	SteamResultCase(k_EResultPasswordRequiredToKickSession,		TEXT("you are already logged in elsewhere, this cached credential login has failed."));
	SteamResultCase(k_EResultAlreadyLoggedInElsewhere,			TEXT("already logged in elsewhere, must wait"));
	SteamResultCase(k_EResultSuspended,							TEXT("operation suspended/paused"));
	SteamResultCase(k_EResultCancelled,							TEXT("operation cancelled"));
	SteamResultCase(k_EResultDataCorruption,					TEXT("operation cancelled due to corrupt data"));
	SteamResultCase(k_EResultDiskFull,							TEXT("operation cancelled due to lack of disk space"));
	SteamResultCase(k_EResultRemoteCallFailed,					TEXT("remote call or IPC call failed"));
	SteamResultCase(k_EResultPasswordUnset,						TEXT("password not verified, as it's unset serverside"));
	SteamResultCase(k_EResultExternalAccountUnlinked,			TEXT("external account not linked to a steam account"));
	SteamResultCase(k_EResultPSNTicketInvalid,					TEXT("PSN ticket invalid"));
	SteamResultCase(k_EResultExternalAccountAlreadyLinked,		TEXT("external account linked to other account"));
	SteamResultCase(k_EResultRemoteFileConflict,				TEXT("sync cannot resume, conflict between local and remote files"));
	SteamResultCase(k_EResultIllegalPassword,					TEXT("requested password not legal"));
	SteamResultCase(k_EResultSameAsPreviousValue,				TEXT("new value same as old"));
	SteamResultCase(k_EResultAccountLogonDenied,				TEXT("account login denied due to 2nd factor auth failure"));
	SteamResultCase(k_EResultCannotUseOldPassword,				TEXT("requested password not legal"));
	SteamResultCase(k_EResultInvalidLoginAuthCode,				TEXT("account login denied, invalid auth code"));
	SteamResultCase(k_EResultAccountLogonDeniedNoMail,			TEXT("account login denied due to 2nd factor auth failure"));
	SteamResultCase(k_EResultHardwareNotCapableOfIPT,			TEXT("???"));
	SteamResultCase(k_EResultIPTInitError,						TEXT("???"));
	SteamResultCase(k_EResultParentalControlRestricted,			TEXT("operation failed due to parental controls"));
	SteamResultCase(k_EResultFacebookQueryError,				TEXT("facebook query returned error"));
	SteamResultCase(k_EResultExpiredLoginAuthCode,				TEXT("account login denied, expired auth code"));
	SteamResultCase(k_EResultIPLoginRestrictionFailed,			TEXT("???"));
	SteamResultCase(k_EResultAccountLockedDown,					TEXT("???"));
	SteamResultCase(k_EResultAccountLogonDeniedVerifiedEmailRequired,	TEXT("???"));
	SteamResultCase(k_EResultNoMatchingURL,						TEXT("no matching URL"));

	default:
		ReturnVal = FString::Printf(TEXT("Unknown EResult result: %i (check Steam SDK)"), (int32)Result);
		break;
	}

	#undef SteamResultCase

	return ReturnVal;
}
