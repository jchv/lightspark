/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <list>

#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/flash/display/Graphics.h"
#include "swf.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/system/flashsystem.h"
#include "parsing/streams.h"
#include "compat.h"
#include "scripting/class.h"
#include "backends/rendering.h"
#include "backends/geometry.h"
#include "backends/input.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Vector.h"

#define FRAME_NOT_FOUND 0xffffffff //Used by getFrameIdBy*

using namespace std;
using namespace lightspark;

std::ostream& lightspark::operator<<(std::ostream& s, const DisplayObject& r)
{
	s << "[" << r.getClass()->class_name << "]";
	if(r.name != BUILTIN_STRINGS::EMPTY)
		s << " name: " << r.name;
	return s;
}

LoaderInfo::LoaderInfo(Class_base* c):EventDispatcher(c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	bytesLoaded(0),bytesTotal(0),sharedEvents(NullRef),
	loader(NullRef),bytesData(NullRef),loadStatus(STARTED),actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	subtype=SUBTYPE_LOADERINFO;
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(c->getSystemState()));
	parameters = _MR(Class<ASObject>::getInstanceS(c->getSystemState()));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(c->getSystemState()));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

LoaderInfo::LoaderInfo(Class_base* c, _R<Loader> l):EventDispatcher(c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	bytesLoaded(0),bytesTotal(0),sharedEvents(NullRef),
	loader(l),bytesData(NullRef),loadStatus(STARTED),actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	subtype=SUBTYPE_LOADERINFO;
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(c->getSystemState()));
	parameters = _MR(Class<ASObject>::getInstanceS(c->getSystemState()));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(c->getSystemState()));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

void LoaderInfo::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("loaderURL","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("loader","",Class<IFunction>::getFunction(c->getSystemState(),_getLoader),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(c->getSystemState(),_getContent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(c->getSystemState(),_getURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesTotal),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytes","",Class<IFunction>::getFunction(c->getSystemState(),_getBytes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("applicationDomain","",Class<IFunction>::getFunction(c->getSystemState(),_getApplicationDomain),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("sharedEvents","",Class<IFunction>::getFunction(c->getSystemState(),_getSharedEvents),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight),GETTER_METHOD,true);
	REGISTER_GETTER(c,parameters);
	REGISTER_GETTER(c,actionScriptVersion);
	REGISTER_GETTER(c,swfVersion);
	REGISTER_GETTER(c,childAllowsParent);
	REGISTER_GETTER(c,contentType);
	REGISTER_GETTER(c,uncaughtErrorEvents);
	REGISTER_GETTER(c,parentAllowsChild);
	REGISTER_GETTER(c,frameRate);
}

ASFUNCTIONBODY_GETTER(LoaderInfo,parameters);
ASFUNCTIONBODY_GETTER(LoaderInfo,actionScriptVersion);
ASFUNCTIONBODY_GETTER(LoaderInfo,childAllowsParent);
ASFUNCTIONBODY_GETTER(LoaderInfo,contentType);
ASFUNCTIONBODY_GETTER(LoaderInfo,swfVersion);
ASFUNCTIONBODY_GETTER(LoaderInfo,uncaughtErrorEvents);
ASFUNCTIONBODY_GETTER(LoaderInfo,parentAllowsChild);
ASFUNCTIONBODY_GETTER(LoaderInfo,frameRate);

void LoaderInfo::buildTraits(ASObject* o)
{
}

bool LoaderInfo::destruct()
{
	sharedEvents.reset();
	loader.reset();
	applicationDomain.reset();
	securityDomain.reset();
	waitedObject.reset();
	bytesData.reset();
	contentType = "application/x-shockwave-flash";
	bytesLoaded = 0;
	bytesTotal = 0;
	loadStatus =STARTED;
	actionScriptVersion = 3;
	swfVersion = 0;
	childAllowsParent = true;
	uncaughtErrorEvents.reset();
	parentAllowsChild =true;
	frameRate =0;
	parameters.reset();
	uncaughtErrorEvents.reset();
	return EventDispatcher::destruct();
}

void LoaderInfo::resetState()
{
	SpinlockLocker l(spinlock);
	bytesLoaded=0;
	bytesTotal=0;
	if(!bytesData.isNull())
		bytesData->setLength(0);
	loadStatus=STARTED;
}

void LoaderInfo::setComplete()
{
	SpinlockLocker l(spinlock);
	if (loadStatus==STARTED)
	{
		sendInit();
	}
}

void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		SpinlockLocker l(spinlock);
		bytesLoaded=b;
		if(getVm(getSystemState()))
		{
			this->incRef();
			getVm(getSystemState())->addEvent(_MR(this),_MR(Class<ProgressEvent>::getInstanceS(getSystemState(),bytesLoaded,bytesTotal)));
		}
		if(loadStatus==INIT_SENT)
		{
			//The clip is also complete now
			if(getVm(getSystemState()))
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"complete")));
			}
			loadStatus=COMPLETE;
		}
	}
}

void LoaderInfo::sendInit()
{
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"init")));
	assert(loadStatus==STARTED);
	loadStatus=INIT_SENT;
	if(bytesTotal && bytesLoaded==bytesTotal)
	{
		//The clip is also complete now
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"complete")));
		loadStatus=COMPLETE;
	}
}

void LoaderInfo::setWaitedObject(_NR<DisplayObject> w)
{
	SpinlockLocker l(spinlock);
	waitedObject = w;
}

void LoaderInfo::objectHasLoaded(_R<DisplayObject> obj)
{
	SpinlockLocker l(spinlock);
	if(waitedObject != obj)
		return;
	if(!loader.isNull() && obj==waitedObject)
		loader->setContent(obj);

	// the init/complete events are sended after the first frame of the waitedObject was executed
	if (loader.isNull() || waitedObject.isNull())
		sendInit();
	else if (!loader->getParent()) // loader has no parent, ensure init/complete events are sended anyway
		loader->getSystemState()->stage->addHiddenObject(waitedObject);
		
	waitedObject.reset();
}

void LoaderInfo::setURL(const tiny_string& _url, bool setParameters)
{
	url=_url;

	//Specs says that parameters should be set from the *main* SWF
	//URL query string, but testing shows that it should be the
	//loaded URL.
	//
	//TODO: the parameters should only be set if the loaded clip
	//uses AS3. See specs.
	if (setParameters)
	{
		parameters = _MR(Class<ASObject>::getInstanceS(getSystemState()));
		SystemState::parseParametersFromURLIntoObject(url, parameters);
	}
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_constructor)
{
	//LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	EventDispatcher::_constructor(ret,sys,obj,NULL,0);
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getLoaderURL)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->loaderURL));
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getContent)
{
	//Use Loader::getContent
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	if(th->loader.isNull())
		asAtomHandler::setUndefined(ret);
	else
	{
		asAtom a = asAtomHandler::fromObject(th->loader.getPtr());
		Loader::_getContent(ret,sys,a,NULL,0);
	}
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getLoader)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	if(th->loader.isNull())
		asAtomHandler::setUndefined(ret);
	else
	{
		th->loader->incRef();
		ret = asAtomHandler::fromObject(th->loader.getPtr());
	}
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getSharedEvents)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	th->sharedEvents->incRef();
	ret = asAtomHandler::fromObject(th->sharedEvents.getPtr());
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getURL)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	ret = asAtomHandler::fromObject(abstract_s(sys,th->url));
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getBytesLoaded)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	asAtomHandler::setUInt(ret,sys,th->bytesLoaded);
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getBytesTotal)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	asAtomHandler::setUInt(ret,sys,th->bytesTotal);
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getBytes)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if (th->bytesData.isNull())
		th->bytesData = _NR<ByteArray>(Class<ByteArray>::getInstanceS(sys));
	if (!th->loader->getContent().isNull())
		th->bytesData->writeObject(th->loader->getContent().getPtr());

	ret = asAtomHandler::fromObject(th->bytesData.getPtr());
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getApplicationDomain)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if(th->applicationDomain.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->applicationDomain->incRef();
	ret = asAtomHandler::fromObject(th->applicationDomain.getPtr());
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getWidth)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	_NR<Loader> l = th->loader;
	if(l.isNull())
	{
		asAtomHandler::setInt(ret,sys,0);
		return;
	}
	_NR<DisplayObject> o=l->getContent();
	if (o.isNull())
	{
		asAtomHandler::setInt(ret,sys,0);
		return;
	}

	asAtomHandler::setInt(ret,sys,o->getNominalWidth());
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getHeight)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	_NR<Loader> l = th->loader;
	if(l.isNull())
	{
		asAtomHandler::setInt(ret,sys,0);
		return;
	}
	_NR<DisplayObject> o=l->getContent();
	if (o.isNull())
	{
		asAtomHandler::setInt(ret,sys,0);
		return;
	}

	asAtomHandler::setInt(ret,sys,o->getNominalHeight());
}

LoaderThread::LoaderThread(_R<URLRequest> request, _R<Loader> ldr)
  : DownloaderThreadBase(request, ldr.getPtr()), loader(ldr), loaderInfo(ldr->getContentLoaderInfo()), source(URL)
{
}

LoaderThread::LoaderThread(_R<ByteArray> _bytes, _R<Loader> ldr)
  : DownloaderThreadBase(NullRef, ldr.getPtr()), bytes(_bytes), loader(ldr), loaderInfo(ldr->getContentLoaderInfo()), source(BYTES)
{
}

void LoaderThread::execute()
{
	assert(source==URL || source==BYTES);

	streambuf *sbuf = 0;
	if(source==URL)
	{
		_R<MemoryStreamCache> cache(_MR(new MemoryStreamCache(loader->getSystemState())));
		if(!createDownloader(cache, loaderInfo, loaderInfo.getPtr(), false))
			return;

		sbuf = cache->createReader();
		
		// Wait for some data, making sure our check for failure is working
		sbuf->sgetc(); // peek one byte
		if(downloader->hasEmptyAnswer())
		{
			LOG(LOG_INFO,"empty answer:"<<url);
			return;
		}

		if(cache->hasFailed()) //Check to see if the download failed for some reason
		{
			LOG(LOG_ERROR, "Loader::execute(): Download of URL failed: " << url);
			loaderInfo->incRef();
			getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<IOErrorEvent>::getInstanceS(loader->getSystemState())));
			loader->incRef();
			getVm(loader->getSystemState())->addEvent(loader,_MR(Class<IOErrorEvent>::getInstanceS(loader->getSystemState())));
			delete sbuf;
			// downloader will be deleted in jobFence
			return;
		}
		loaderInfo->incRef();
		getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<Event>::getInstanceS(loader->getSystemState(),"open")));
	}
	else if(source==BYTES)
	{
		assert_and_throw(bytes->bytes);

		loaderInfo->incRef();
		getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<Event>::getInstanceS(loader->getSystemState(),"open")));
		loaderInfo->setBytesTotal(bytes->getLength());
		loaderInfo->setBytesLoaded(bytes->getLength());

		sbuf = new bytes_buf(bytes->bytes,bytes->getLength());
	}

	istream s(sbuf);
	ParseThread local_pt(s,loaderInfo->applicationDomain,loaderInfo->securityDomain,loader.getPtr(),url.getParsedURL());
	local_pt.execute();

	// Delete the bytes container (cache reader or bytes_buf)
	delete sbuf;
	sbuf = NULL;
	if (source==URL) {
		//Acquire the lock to ensure consistency in threadAbort
		SpinlockLocker l(downloaderLock);
		if(downloader)
			loaderInfo->getSystemState()->downloadManager->destroy(downloader);
		downloader=NULL;
	}

	bytes.reset();

	_NR<DisplayObject> obj=local_pt.getParsedObject();
	if(obj.isNull())
	{
		// The stream did not contain RootMovieClip or Bitmap
		if(!threadAborting)
		{
			loaderInfo->incRef();
			getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<IOErrorEvent>::getInstanceS(loader->getSystemState())));
		}
		return;
	}
	if (loader.getPtr() && local_pt.getRootMovie() && local_pt.getRootMovie()->hasFinishedLoading())
	{
		if (local_pt.getRootMovie() != loader->getSystemState()->mainClip )
		{
			local_pt.getRootMovie()->incRef();
			loader->setContent(_MR(local_pt.getRootMovie()));
			if (loader->getContentLoaderInfo().getPtr())
				loader->getContentLoaderInfo()->setComplete();
		}
	}
}

ASFUNCTIONBODY_ATOM(Loader,_constructor)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	DisplayObjectContainer::_constructor(ret,sys,obj,NULL,0);
	th->contentLoaderInfo->setLoaderURL(th->getSystemState()->mainClip->getOrigin().getParsedURL());
	th->uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(sys));
}

ASFUNCTIONBODY_ATOM(Loader,_getContent)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	SpinlockLocker l(th->spinlock);
	_NR<ASObject> res=th->content;
	if(res.isNull())
	{
		asAtomHandler::setUndefined(ret);
		return;
	}

	res->incRef();
	ret = asAtomHandler::fromObject(res.getPtr());
}

ASFUNCTIONBODY_ATOM(Loader,_getContentLoaderInfo)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	th->contentLoaderInfo->incRef();
	ret = asAtomHandler::fromObject(th->contentLoaderInfo.getPtr());
}

ASFUNCTIONBODY_ATOM(Loader,close)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
 	SpinlockLocker l(th->spinlock);
	for (auto j=th->jobs.begin(); j!=th->jobs.end(); j++)
		(*j)->threadAbort();
}

ASFUNCTIONBODY_ATOM(Loader,load)
{
	Loader* th=asAtomHandler::as<Loader>(obj);

	th->unload();
	_NR<URLRequest> r;
	_NR<LoaderContext> context;
	ARG_UNPACK_ATOM (r)(context, NullRef);
	th->loadIntern(r.getPtr(),context.getPtr());
}
void Loader::loadIntern(URLRequest* r, LoaderContext* context)
{
	this->url=r->getRequestURL();
	this->contentLoaderInfo->setURL(this->url.getParsedURL());
	this->contentLoaderInfo->resetState();
	//Check if a security domain has been manually set
	_NR<SecurityDomain> secDomain;
	_NR<SecurityDomain> curSecDomain=ABCVm::getCurrentSecurityDomain(getVm(this->getSystemState())->currentCallContext);
	if(context)
	{
		if (!context->securityDomain.isNull())
		{
			//The passed domain must be the current one. See Loader::load specs.
			if(context->securityDomain!=curSecDomain)
				throw Class<SecurityError>::getInstanceS(this->getSystemState(),"SecurityError: securityDomain must be current one");
			secDomain=curSecDomain;
		}

		bool sameDomain = (secDomain == curSecDomain);
		this->allowCodeImport = !sameDomain || context->getAllowCodeImport();

		if (!context->parameters.isNull())
			this->contentLoaderInfo->setParameters(context->parameters);
	}
	//Default is to create a child ApplicationDomain if the file is in the same security context
	//otherwise create a child of the system domain. If the security domain is different
	//the passed applicationDomain is ignored
	_R<RootMovieClip> currentRoot=getVm(this->getSystemState())->currentCallContext->mi->context->root;
	// empty origin is possible if swf is loaded by loadBytes()
	if(currentRoot->getOrigin().isEmpty() || currentRoot->getOrigin().getHostname()==this->url.getHostname() || !secDomain.isNull())
	{
		//Same domain
		_NR<ApplicationDomain> parentDomain = currentRoot->applicationDomain;
		//Support for LoaderContext
		if(!context || context->applicationDomain.isNull())
			this->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(this->getSystemState(),parentDomain));
		else
			this->contentLoaderInfo->applicationDomain = context->applicationDomain;
		this->contentLoaderInfo->securityDomain = curSecDomain;
	}
	else
	{
		//Different domain
		_NR<ApplicationDomain> parentDomain =  this->getSystemState()->systemDomain;
		this->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(this->getSystemState(),parentDomain));
		this->contentLoaderInfo->securityDomain = _MR(Class<SecurityDomain>::getInstanceS(this->getSystemState()));
	}

	if(!this->url.isValid())
	{
		//Notify an error during loading
		this->incRef();
		this->getSystemState()->currentVm->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS(this->getSystemState())));
		return;
	}

	SecurityManager::checkURLStaticAndThrow(this->url, ~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);

	if (context && context->getCheckPolicyFile())
	{
		//TODO: this should be async as it could block if invoked from ExternalInterface
		SecurityManager::EVALUATIONRESULT evaluationResult;
		evaluationResult = this->getSystemState()->securityManager->evaluatePoliciesURL(this->url, true);
		if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		{
			// should this dispatch SecurityErrorEvent instead of throwing?
			throw Class<SecurityError>::getInstanceS(this->getSystemState(),
				"SecurityError: connection to domain not allowed by securityManager");
		}
	}

	this->incRef();
	r->incRef();
	LoaderThread *thread=new LoaderThread(_MR(r), _MR(this));

	SpinlockLocker l(this->spinlock);
	this->jobs.push_back(thread);
	this->getSystemState()->addJob(thread);
}

ASFUNCTIONBODY_ATOM(Loader,loadBytes)
{
	Loader* th=asAtomHandler::as<Loader>(obj);


	th->unload();

	_NR<ByteArray> bytes;
	_NR<LoaderContext> context;
	ARG_UNPACK_ATOM (bytes)(context, NullRef);

	_NR<ApplicationDomain> parentDomain = ABCVm::getCurrentApplicationDomain(getVm(th->getSystemState())->currentCallContext);
	if(context.isNull() || context->applicationDomain.isNull())
		th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(sys,parentDomain));
	else
		th->contentLoaderInfo->applicationDomain = context->applicationDomain;
	//Always loaded in the current security domain
	_NR<SecurityDomain> curSecDomain=ABCVm::getCurrentSecurityDomain(getVm(th->getSystemState())->currentCallContext);
	th->contentLoaderInfo->securityDomain = curSecDomain;

	th->allowCodeImport = context.isNull() || context->getAllowCodeImport();

	if (!context.isNull() && !context->parameters.isNull())
		th->contentLoaderInfo->setParameters(context->parameters);

	if(bytes->getLength()!=0)
	{
		th->incRef();
		LoaderThread *thread=new LoaderThread(_MR(bytes), _MR(th));
		SpinlockLocker l(th->spinlock);
		th->jobs.push_back(thread);
		sys->addJob(thread);
	}
	else
		LOG(LOG_INFO, "Empty ByteArray passed to Loader.loadBytes");
}

ASFUNCTIONBODY_ATOM(Loader,_unload)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	th->unload();
}
ASFUNCTIONBODY_ATOM(Loader,_unloadAndStop)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	th->unload();
	LOG(LOG_NOT_IMPLEMENTED,"unloadAndStop does not execute any stopping actions");
	/* TODO: (taken from specs)
	Sounds are stopped.
	Stage event listeners are removed.
	Event listeners for enterFrame, frameConstructed, exitFrame, activate and deactivate are removed.
	Timers are stopped.
	Camera and Microphone instances are detached
	Movie clips are stopped.
	*/
}

void Loader::unload()
{
	DisplayObject* content_copy = nullptr;
	{
		SpinlockLocker l(spinlock);
		for (auto j=jobs.begin(); j!=jobs.end(); j++)
			(*j)->threadAbort();

		content_copy=content.getPtr();
		content.reset();
	}
	
	if(loaded)
	{
		contentLoaderInfo->incRef();
		getVm(getSystemState())->addEvent(contentLoaderInfo,_MR(Class<Event>::getInstanceS(getSystemState(),"unload")));
		loaded=false;
	}

	// removeChild may execute AS code, release the lock before
	// calling
	if(content_copy)
		_removeChild(content_copy);

	contentLoaderInfo->resetState();
}

void Loader::finalize()
{
	DisplayObjectContainer::finalize();
	content.reset();
	contentLoaderInfo.reset();
}

Loader::Loader(Class_base* c):DisplayObjectContainer(c),content(NullRef),contentLoaderInfo(NullRef),loaded(false), allowCodeImport(true),uncaughtErrorEvents(NullRef)
{
	incRef();
	contentLoaderInfo=_MR(Class<LoaderInfo>::getInstanceS(c->getSystemState(),_MR(this)));
}

Loader::~Loader()
{
}

void Loader::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("contentLoaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getContentLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(c->getSystemState(),_getContent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",Class<IFunction>::getFunction(c->getSystemState(),loadBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unload","",Class<IFunction>::getFunction(c->getSystemState(),_unload),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unloadAndStop","",Class<IFunction>::getFunction(c->getSystemState(),_unloadAndStop),NORMAL_METHOD,true);
	REGISTER_GETTER(c,uncaughtErrorEvents);
}

ASFUNCTIONBODY_GETTER(Loader,uncaughtErrorEvents);

void Loader::threadFinished(IThreadJob* finishedJob)
{
	SpinlockLocker l(spinlock);
	jobs.remove(finishedJob);
	delete finishedJob;
}

void Loader::buildTraits(ASObject* o)
{
}

void Loader::setContent(_R<DisplayObject> o)
{
	{
		Locker l(mutexDisplayList);
		dynamicDisplayList.clear();
	}

	{
		SpinlockLocker l(spinlock);
		content=o;
		content->isLoadedRoot = true;
		loaded=true;
	}

	// _addChild may cause AS code to run, release locks beforehand.
	_addChildAt(o, 0);
}

Sprite::Sprite(Class_base* c):DisplayObjectContainer(c),TokenContainer(this, this->getSystemState()->spriteTokenMemory),graphics(NullRef),dragged(false),buttonMode(false),useHandCursor(false)
{
	subtype=SUBTYPE_SPRITE;
}

bool Sprite::destruct()
{
	resetToStart();
	graphics.reset();
	hitArea.reset();
	hitTarget.reset();
	dragged = false;
	buttonMode = false;
	useHandCursor = false;
	tokens.clear();
	return DisplayObjectContainer::destruct();
}

void Sprite::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(c->getSystemState(),_getGraphics),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",Class<IFunction>::getFunction(c->getSystemState(),_startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",Class<IFunction>::getFunction(c->getSystemState(),_stopDrag),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, buttonMode);
	REGISTER_GETTER_SETTER(c, hitArea);
	REGISTER_GETTER_SETTER(c, useHandCursor);
	c->setDeclaredMethodByQName("soundTransform","",Class<IFunction>::getFunction(c->getSystemState(),getSoundTransform),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("soundTransform","",Class<IFunction>::getFunction(c->getSystemState(),setSoundTransform),SETTER_METHOD,true);
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Sprite, buttonMode);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Sprite, useHandCursor);

void Sprite::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Sprite,_startDrag)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	bool lockCenter = false;
	const RECT* bounds = NULL;
	ARG_UNPACK_ATOM(lockCenter,false);
	if(argslen > 1)
	{
		Rectangle* rect = Class<Rectangle>::cast(asAtomHandler::getObject(args[1]));
		if(!rect)
			throw Class<ArgumentError>::getInstanceS(sys,"Wrong type");
		bounds = new RECT(rect->getRect());
	}

	Vector2f offset;
	if(!lockCenter)
	{
		offset = -th->getParent()->getLocalMousePos();
		offset += th->getXY();
	}

	th->incRef();
	sys->getInputThread()->startDrag(_MR(th), bounds, offset);
}

ASFUNCTIONBODY_ATOM(Sprite,_stopDrag)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	sys->getInputThread()->stopDrag(th);
}

ASFUNCTIONBODY_GETTER(Sprite, hitArea);

ASFUNCTIONBODY_ATOM(Sprite,_setter_hitArea)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	_NR<Sprite> value;
	ARG_UNPACK_ATOM(value);

	if (!th->hitArea.isNull())
		th->hitArea->hitTarget.reset();

	th->hitArea = value;
	if (!th->hitArea.isNull())
	{
		th->incRef();
		th->hitArea->hitTarget = _MNR(th);
	}
}

ASFUNCTIONBODY_ATOM(Sprite,getSoundTransform)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	if (th->sound && th->sound->soundTransform)
	{
		ret = asAtomHandler::fromObject(th->sound->soundTransform.getPtr());
		ASATOM_INCREF(ret);
	}
	else
		asAtomHandler::setNull(ret);
}
ASFUNCTIONBODY_ATOM(Sprite,setSoundTransform)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	if (th->sound)
	{
		if (argslen == 0 || !asAtomHandler::is<SoundTransform>(args[0]))
			th->sound->soundTransform.reset();
		else
			th->sound->soundTransform =  _MR(asAtomHandler::getObject(args[0])->as<SoundTransform>());
	}
}

bool DisplayObjectContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret = false;

	if(dynamicDisplayList.empty())
		return false;

	Locker l(mutexDisplayList);
	std::vector<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		number_t txmin,txmax,tymin,tymax;
		if((*it)->getBounds(txmin,txmax,tymin,tymax,(*it)->getMatrix()))
		{
			if(ret==true)
			{
				xmin = imin(xmin,txmin);
				xmax = imax(xmax,txmax);
				ymin = imin(ymin,tymin);
				ymax = imax(ymax,tymax);
			}
			else
			{
				xmin=txmin;
				xmax=txmax;
				ymin=tymin;
				ymax=tymax;
				ret=true;
			}
		}
	}
	return ret;
}

bool Sprite::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret;
	ret = DisplayObjectContainer::boundsRect(xmin,xmax,ymin,ymax);
	number_t txmin,txmax,tymin,tymax;
	if(TokenContainer::boundsRect(txmin,txmax,tymin,tymax))
	{
		if(ret==true)
		{
			xmin = imin(xmin,txmin);
			xmax = imax(xmax,txmax);
			ymin = imin(ymin,tymin);
			ymax = imax(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
		}
		ret=true;
	}
	return ret;
}

void Sprite::requestInvalidation(InvalidateQueue* q)
{
	DisplayObjectContainer::requestInvalidation(q);
	TokenContainer::requestInvalidation(q);
}

void DisplayObjectContainer::renderImpl(RenderContext& ctxt) const
{
	Locker l(mutexDisplayList);
	//Now draw also the display list
	std::vector<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		//Skip the drawing of masks
		if((*it)->isMask() || (*it)->ClipDepth)
			continue;
		(*it)->Render(ctxt);
	}
}

void DisplayObjectContainer::LegacyChildEraseDeletionMarked()
{
	auto it = legacyChildrenMarkedForDeletion.begin();
	while (it != legacyChildrenMarkedForDeletion.end())
	{
		deleteLegacyChildAt(*it);
		it = legacyChildrenMarkedForDeletion.erase(it);
	}
}

void DisplayObjectContainer::LegacyChildRemoveDeletionMark(int32_t depth)
{
	auto it = legacyChildrenMarkedForDeletion.find(depth);
	if (it != legacyChildrenMarkedForDeletion.end())
		legacyChildrenMarkedForDeletion.erase(it);
}


void Sprite::renderImpl(RenderContext& ctxt) const
{
	//Draw the dynamically added graphics, if any
	defaultRender(ctxt);

	DisplayObjectContainer::renderImpl(ctxt);
}

/*
Subclasses of DisplayObjectContainer must still check
isHittable() to see if they should send out events.
*/
_NR<DisplayObject> DisplayObjectContainer::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;
	//Test objects added at runtime, in reverse order
	Locker l(mutexDisplayList);
	std::vector<_R<DisplayObject>>::const_reverse_iterator j=dynamicDisplayList.rbegin();
	for(;j!=dynamicDisplayList.rend();++j)
	{
		//Don't check masks
		if((*j)->isMask())
			continue;

		if(!(*j)->getMatrix().isInvertible())
			continue; /* The object is shrunk to zero size */

		number_t localX, localY;
		(*j)->getMatrix().getInverted().multiply2D(x,y,localX,localY);
		this->incRef();
		ret=(*j)->hitTest(_MR(this), localX,localY, type,interactiveObjectsOnly);
		if(!ret.isNull())
		{
			if (interactiveObjectsOnly && !ret->is<InteractiveObject>())
			{
				if (this->is<RootMovieClip>())
					continue;
				this->incRef();
				ret = _MNR(this);
				return ret;
			}
			break;
		}
	}
	// only check interactive objects
	if(ret && interactiveObjectsOnly && !ret->is<InteractiveObject>())
		ret.reset();
	
	/* When mouseChildren is false, we should get all events of our children */
	if(ret && !mouseChildren)
	{
		if (!isHittable(type))
			ret.reset();
		else
		{
			this->incRef();
			ret = _MNR(this);
		}
	}
	return ret;
}

_NR<DisplayObject> Sprite::hitTestImpl(_NR<DisplayObject>, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	//Did we hit a children?
	_NR<DisplayObject> ret = NullRef;
	if (dragged) // no hitting when in drag/drop mode
		return ret;
	this->incRef();
	ret = DisplayObjectContainer::hitTestImpl(_MR(this),x,y, type,interactiveObjectsOnly);

	if (ret.isNull() && hitArea.isNull())
	{
		//The coordinates are locals
		this->incRef();
		ret = TokenContainer::hitTestImpl(_MR(this),x,y, type);

		if (!ret.isNull())  //Did we hit the sprite?
		{
			if (!hitTarget.isNull())
			{
				//Another Sprite has registered us
				//as its hitArea -> relay the hit
				if (hitTarget->isHittable(type))
					ret = hitTarget;
				else
					ret.reset();
			}
			else if (!isHittable(type))
			{
				//Hit ignored due to a disabled HIT_TYPE
				ret.reset();
			}
		}
	}

	return ret;
}

void Sprite::resetToStart()
{
	if (sound)
	{
		sound->threadAbort();
		sound.reset();
	}
}

void Sprite::setSound(SoundChannel *s)
{
	sound = _MR(s);
}

void Sprite::appendSound(unsigned char *buf, int len)
{
	if (sound)
		sound->appendStreamBlock(buf,len);
}

void Sprite::checkSound()
{
	if (sound)
		sound->play(); 
	
}

void Sprite::markSoundFinished()
{
	if (sound)
		sound->markFinished();
}

ASFUNCTIONBODY_ATOM(Sprite,_constructor)
{
	//Sprite* th=Class<Sprite>::cast(obj);
	DisplayObjectContainer::_constructor(ret,sys,obj,NULL,0);
}

_NR<Graphics> Sprite::getGraphics()
{
	//Probably graphics is not used often, so create it here
	if(graphics.isNull())
		graphics=_MR(Class<Graphics>::getInstanceS(getSystemState(),this));
	return graphics;
}

ASFUNCTIONBODY_ATOM(Sprite,_getGraphics)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	_NR<Graphics> g = th->getGraphics();

	g->incRef();
	ret = asAtomHandler::fromObject(g.getPtr());
}

FrameLabel::FrameLabel(Class_base* c):ASObject(c)
{
}

FrameLabel::FrameLabel(Class_base* c, const FrameLabel_data& data):ASObject(c),FrameLabel_data(data)
{
}

void FrameLabel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("frame","",Class<IFunction>::getFunction(c->getSystemState(),_getFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(c->getSystemState(),_getName),GETTER_METHOD,true);
}

void FrameLabel::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(FrameLabel,_getFrame)
{
	FrameLabel* th=asAtomHandler::as<FrameLabel>(obj);
	asAtomHandler::setUInt(ret,sys,th->frame);
}

ASFUNCTIONBODY_ATOM(FrameLabel,_getName)
{
	FrameLabel* th=asAtomHandler::as<FrameLabel>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->name));
}

/*
 * Adds a frame label to the internal vector and keep
 * the vector sorted with respect to frame
 */
void Scene_data::addFrameLabel(uint32_t frame, const tiny_string& label)
{
	for(vector<FrameLabel_data>::iterator j=labels.begin();
		j != labels.end();++j)
	{
		FrameLabel_data& fl = *j;
		if(fl.frame == frame)
		{
			LOG(LOG_INFO,"existing frame label found:"<<fl.name<<", new value:"<<label);
			fl.name = label;
			return;
		}
		else if(fl.frame > frame)
		{
			labels.insert(j,FrameLabel_data(frame,label));
			return;
		}
	}

	labels.push_back(FrameLabel_data(frame,label));
}

Scene::Scene(Class_base* c):ASObject(c)
{
}

Scene::Scene(Class_base* c, const Scene_data& data, uint32_t _numFrames):ASObject(c),Scene_data(data),numFrames(_numFrames)
{
}

void Scene::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("labels","",Class<IFunction>::getFunction(c->getSystemState(),_getLabels),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(c->getSystemState(),_getName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numFrames","",Class<IFunction>::getFunction(c->getSystemState(),_getNumFrames),GETTER_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Scene,_getLabels)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(sys);
	res->resize(th->labels.size());
	for(size_t i=0; i<th->labels.size(); ++i)
	{
		asAtom v = asAtomHandler::fromObject(Class<FrameLabel>::getInstanceS(sys,th->labels[i]));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Scene,_getName)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->name));
}

ASFUNCTIONBODY_ATOM(Scene,_getNumFrames)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	ret = asAtomHandler::fromUInt(th->numFrames);
}

void Frame::destroyTags()
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		delete (*it);
	auto it2=avm1actions.begin();
	for(;it2!=avm1actions.end();++it2)
		delete (*it2);
}

void Frame::execute(DisplayObjectContainer* displayList)
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		(*it)->execute(displayList);
	displayList->checkClipDepth();

}
void Frame::AVM1executeActions(MovieClip* clip, bool avm1initactionsdone)
{
	if (!avm1initactionsdone)
	{
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" initActions "<< clip->toDebugString());
		auto it1=avm1initactions.begin();
		for(;it1!=avm1initactions.end();++it1)
			(*it1)->execute(clip,&avm1context);
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<clip->state.FP<<" initActions done" << clip->toDebugString());
	}

	auto it2=avm1actions.begin();
	for(;it2!=avm1actions.end();++it2)
		(*it2)->execute(clip,&avm1context);
}

FrameContainer::FrameContainer():framesLoaded(0)
{
	frames.emplace_back(Frame());
	scenes.resize(1);
}

FrameContainer::FrameContainer(const FrameContainer& f):frames(f.frames),scenes(f.scenes),framesLoaded((int)f.framesLoaded)
{
}

/* This runs in parser thread context,
 * but no locking is needed here as it only accesses the last frame.
 * See comment on the 'frames' member. */
void FrameContainer::addToFrame(DisplayListTag* t)
{
	frames.back().blueprint.push_back(t);
}
void FrameContainer::addAvm1ActionToFrame(AVM1ActionTag* t)
{
	frames.back().avm1actions.push_back(t);
}
void FrameContainer::addAvm1InitActionToFrame(AVM1InitActionTag* t)
{
	frames.back().avm1initactions.push_back(t);
}

/**
 * Find the scene to which the given frame belongs and
 * adds the frame label to that scene.
 * The labels of the scene will stay sorted by frame.
 */
void FrameContainer::addFrameLabel(uint32_t frame, const tiny_string& label)
{
	for(size_t i=0; i<scenes.size();++i)
	{
		if(frame < scenes[i].startframe)
		{
			scenes[i-1].addFrameLabel(frame,label);
			return;
		}
	}
	scenes.back().addFrameLabel(frame,label);
}

void MovieClip::sinit(Class_base* c)
{
	CLASS_SETUP(c, Sprite, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("currentFrame","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("totalFrames","",Class<IFunction>::getFunction(c->getSystemState(),_getTotalFrames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("framesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getFramesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFrameLabel","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrameLabel),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabel","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentLabel),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabels","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentLabels),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scenes","",Class<IFunction>::getFunction(c->getSystemState(),_getScenes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentScene","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentScene),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prevFrame","",Class<IFunction>::getFunction(c->getSystemState(),prevFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",Class<IFunction>::getFunction(c->getSystemState(),nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addFrameScript","",Class<IFunction>::getFunction(c->getSystemState(),addFrameScript),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, enabled);
}

ASFUNCTIONBODY_GETTER_SETTER(MovieClip, enabled);

void MovieClip::buildTraits(ASObject* o)
{
}

MovieClip::MovieClip(Class_base* c):Sprite(c),fromDefineSpriteTag(UINT32_MAX),frameScriptToExecute(UINT32_MAX),actions(0),totalFrames_unreliable(1),enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
	currentframeIterator=frames.end();
}

MovieClip::MovieClip(Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID):Sprite(c),FrameContainer(f),fromDefineSpriteTag(defineSpriteTagID),frameScriptToExecute(UINT32_MAX),actions(0),totalFrames_unreliable(frames.size()),enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
	currentframeIterator=frames.end();
	//For sprites totalFrames_unreliable is the actual frame count
	//For the root movie, it's the frame count from the header
}

bool MovieClip::destruct()
{
	frames.clear();
	currentframeIterator=frames.end();
	setFramesLoaded(0);
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASATOM_DECREF(it->second);
		it++;
	}
	frameScripts.clear();
	
	fromDefineSpriteTag = UINT32_MAX;
	frameScriptToExecute = UINT32_MAX;
	totalFrames_unreliable = 1;
	avm1initactionsdone = false;
	avm1variables.clear();
	variablebindings.clear();
	avm1functions.clear();
	frameinitactionsdone.clear();

	frames.clear();
	scenes.clear();
	setFramesLoaded(0);
	frames.emplace_back(Frame());
	scenes.resize(1);
	
	enabled = true;
	return Sprite::destruct();
}

/* Returns a Scene_data pointer for a scene called sceneName, or for
 * the current scene if sceneName is empty. Returns NULL, if not found.
 */
const Scene_data *MovieClip::getScene(const tiny_string &sceneName) const
{
	if (sceneName.empty())
	{
		return &scenes[getCurrentScene()];
	}
	else
	{
		//Find scene by name
		for (auto it=scenes.begin(); it!=scenes.end(); ++it)
		{
			if (it->name == sceneName)
				return &*it;
		}
	}

	return NULL;  //Not found!
}

/* Return global frame index for a named frame. If sceneName is not
 * empty, return a frame only if it belong to the named scene.
 */
uint32_t MovieClip::getFrameIdByLabel(const tiny_string& label, const tiny_string& sceneName) const
{
	if (sceneName.empty())
	{
		//Find frame in any scene
		for(size_t i=0;i<scenes.size();++i)
		{
			for(size_t j=0;j<scenes[i].labels.size();++j)
				if(scenes[i].labels[j].name == label)
					return scenes[i].labels[j].frame;
		}
	}
	else
	{
		//Find frame in the named scene only
		const Scene_data *scene = getScene(sceneName);
		if (scene)
		{
			for(size_t j=0;j<scene->labels.size();++j)
			{
				if(scene->labels[j].name == label)
					return scene->labels[j].frame;
			}
		}
	}

	return FRAME_NOT_FOUND;
}

/* Return global frame index for frame i (zero-based) in a scene
 * called sceneName. If sceneName is empty, use the current scene.
 */
uint32_t MovieClip::getFrameIdByNumber(uint32_t i, const tiny_string& sceneName) const
{
	const Scene_data *sceneData = getScene(sceneName);
	if (!sceneData)
		return FRAME_NOT_FOUND;

	//Should we check if the scene has at least i frames?
	return sceneData->startframe + i;
}

ASFUNCTIONBODY_ATOM(MovieClip,addFrameScript)
{
	MovieClip* th=Class<MovieClip>::cast(asAtomHandler::getObject(obj));
	assert_and_throw(argslen>=2 && argslen%2==0);

	for(uint32_t i=0;i<argslen;i+=2)
	{
		uint32_t frame=asAtomHandler::toInt(args[i]);

		if(!asAtomHandler::isFunction(args[i+1]))
		{
			LOG(LOG_ERROR,_("Not a function"));
			return;
		}
		ASATOM_INCREF(args[i+1]);
		th->frameScripts[frame]=args[i+1];
	}
}

ASFUNCTIONBODY_ATOM(MovieClip,swapDepths)
{
	LOG(LOG_NOT_IMPLEMENTED,_("Called swapDepths"));
}

ASFUNCTIONBODY_ATOM(MovieClip,stop)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->state.stop_FP=true;
	th->state.next_FP=th->state.FP;
}

ASFUNCTIONBODY_ATOM(MovieClip,play)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->state.stop_FP=false;
}

void MovieClip::gotoAnd(asAtom* args, const unsigned int argslen, bool stop)
{
	uint32_t next_FP;
	tiny_string sceneName;
	assert_and_throw(argslen==1 || argslen==2);
	if(argslen==2 && getSystemState()->mainClip->usesActionScript3)
	{
		sceneName = asAtomHandler::toString(args[1],getSystemState());
	}
	if(asAtomHandler::isString(args[0]))
	{
		uint32_t dest=getFrameIdByLabel(asAtomHandler::toString(args[0],getSystemState()), sceneName);
		if(dest==FRAME_NOT_FOUND)
		{
			dest= 0;
			LOG(LOG_ERROR, (stop ? "gotoAndStop: label not found:" : "gotoAndPlay: label not found:") <<asAtomHandler::toString(args[0],getSystemState())<<" in scene "<<sceneName);
//			throwError<ArgumentError>(kInvalidArgumentError,asAtomHandler::toString(args[0],));
		}

		next_FP = dest;
	}
	else
	{
		uint32_t inFrameNo = asAtomHandler::toInt(args[0]);
		if(inFrameNo == 0)
			inFrameNo = 1;

		next_FP = getFrameIdByNumber(inFrameNo-1, sceneName);
		if(next_FP > getFramesLoaded())
		{
			LOG(LOG_ERROR, next_FP << "= next_FP >= state.max_FP = " << getFramesLoaded() << " on "<<this->getTagID());
			/* spec says we should throw an error, but then YT breaks */
			//throwError<ArgumentError>(kInvalidArgumentError,stop ? "gotoAndStop: frame not found" : "gotoAndPlay: frame not found");
			next_FP = getFramesLoaded()-1;
		}
	}
	state.next_FP = next_FP;
	state.explicit_FP = true;
	state.stop_FP = stop;

	if (!this->isOnStage())
	{
		advanceFrame();
		initFrame();
		this->incRef();
		this->getSystemState()->currentVm->addEvent(NullRef, _MR(new (this->getSystemState()->unaccountedMemory) ExecuteFrameScriptEvent(_MR(this))));
	}
	else if (state.creatingframe) // this can occur if we are between the advanceFrame and the initFrame calls (that means we are currently executing an enterFrame event)
		advanceFrame();
}

void MovieClip::AVM1gotoFrameLabel(const tiny_string& label)
{
	uint32_t dest=getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_ERROR, "gotoFrameLabel: label not found:" <<label);
		return;
	}
	AVM1gotoFrame(dest, true, true);
}
void MovieClip::AVM1gotoFrame(int frame, bool stop, bool switchplaystate)
{
	state.next_FP = frame;
	state.explicit_FP = true;
	bool advance = true;
	if (switchplaystate)
	{
		if (!stop && state.stop_FP)
		{
			// play called and we have stopped before, no need to advance
			advance = false;
		}
		state.stop_FP = stop;
	}
	if (advance) 
		advanceFrame();
}

ASFUNCTIONBODY_ATOM(MovieClip,gotoAndStop)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->gotoAnd(args,argslen,true);
}

ASFUNCTIONBODY_ATOM(MovieClip,gotoAndPlay)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->gotoAnd(args,argslen,false);
}

ASFUNCTIONBODY_ATOM(MovieClip,nextFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	th->state.next_FP = th->state.FP+1;
	th->state.explicit_FP=true;
	if (!th->isOnStage())
	{
		th->advanceFrame();
		th->initFrame();
		th->incRef();
		sys->currentVm->addEvent(NullRef, _MR(new (sys->unaccountedMemory) ExecuteFrameScriptEvent(_MR(th))));
	}
	else if (th->state.creatingframe) // this can occur if we are between the advanceFrame and the initFrame calls (that means we are currently executing an enterFrame event)
		th->advanceFrame();

}

ASFUNCTIONBODY_ATOM(MovieClip,prevFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	th->state.next_FP = th->state.FP-1;
	th->state.explicit_FP=true;
	if (!th->isOnStage())
	{
		th->advanceFrame();
		th->initFrame();
		th->incRef();
		sys->currentVm->addEvent(NullRef, _MR(new (sys->unaccountedMemory) ExecuteFrameScriptEvent(_MR(th))));
	}
	else if (th->state.creatingframe) // this can occur if we are between the advanceFrame and the initFrame calls (that means we are currently executing an enterFrame event)
		th->advanceFrame();
}

ASFUNCTIONBODY_ATOM(MovieClip,_getFramesLoaded)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,sys,th->getFramesLoaded());
}

ASFUNCTIONBODY_ATOM(MovieClip,_getTotalFrames)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,sys,th->totalFrames_unreliable);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getScenes)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(sys);
	res->resize(th->scenes.size());
	uint32_t numFrames;
	for(size_t i=0; i<th->scenes.size(); ++i)
	{
		if(i == th->scenes.size()-1)
			numFrames = th->totalFrames_unreliable - th->scenes[i].startframe;
		else
			numFrames = th->scenes[i].startframe - th->scenes[i+1].startframe;
		asAtom v = asAtomHandler::fromObject(Class<Scene>::getInstanceS(sys,th->scenes[i],numFrames));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

uint32_t MovieClip::getCurrentScene() const
{
	for(size_t i=0;i<scenes.size();++i)
	{
		if(state.FP < scenes[i].startframe)
			return i-1;
	}
	return scenes.size()-1;
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentScene)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	uint32_t numFrames;
	uint32_t curScene = th->getCurrentScene();
	if(curScene == th->scenes.size()-1)
		numFrames = th->totalFrames_unreliable - th->scenes[curScene].startframe;
	else
		numFrames = th->scenes[curScene].startframe - th->scenes[curScene+1].startframe;

	ret = asAtomHandler::fromObject(Class<Scene>::getInstanceS(sys,th->scenes[curScene],numFrames));
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	//currentFrame is 1-based and relative to current scene
	if (th->state.explicit_FP)
		// if frame is set explicitly, the currentframe property should be set to next_FP, even if it is not displayed yet
		asAtomHandler::setInt(ret,sys,th->state.next_FP+1 - th->scenes[th->getCurrentScene()].startframe);
	else
		asAtomHandler::setInt(ret,sys,th->state.FP+1 - th->scenes[th->getCurrentScene()].startframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrameLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	for(size_t i=0;i<th->scenes.size();++i)
	{
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
			if(th->scenes[i].labels[j].frame == th->state.FP)
			{
				ret = asAtomHandler::fromObject(abstract_s(sys,th->scenes[i].labels[j].name));
				return;
			}
	}
	asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	tiny_string label;
	for(size_t i=0;i<th->scenes.size();++i)
	{
		if(th->scenes[i].startframe > th->state.FP)
			break;
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
		{
			if(th->scenes[i].labels[j].frame > th->state.FP)
				break;
			if(!th->scenes[i].labels[j].name.empty())
				label = th->scenes[i].labels[j].name;
		}
	}

	if(label.empty())
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromObject(abstract_s(sys,label));
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabels)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Scene_data& sc = th->scenes[th->getCurrentScene()];

	Array* res = Class<Array>::getInstanceSNoArgs(sys);
	res->resize(sc.labels.size());
	for(size_t i=0; i<sc.labels.size(); ++i)
	{
		asAtom v = asAtomHandler::fromObject(Class<FrameLabel>::getInstanceS(sys,sc.labels[i]));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(MovieClip,_constructor)
{
	Sprite::_constructor(ret,sys,obj,NULL,0);
/*	th->setVariableByQName("swapDepths","",Class<IFunction>::getFunction(c->getSystemState(),swapDepths));
	th->setVariableByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),createEmptyMovieClip));*/
}

void MovieClip::addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name)
{
	if(sceneNo == 0)
	{
		//we always have one scene, but this call may set its name
		scenes[0].name = name;
	}
	else
	{
		assert(scenes.size() == sceneNo);
		scenes.resize(sceneNo+1);
		scenes[sceneNo].name = name;
		scenes[sceneNo].startframe = startframe;
	}
}

void MovieClip::afterLegacyInsert()
{
}

void MovieClip::afterLegacyDelete(DisplayObjectContainer *par)
{
	getSystemState()->stage->AVM1RemoveMouseListener(this);
	getSystemState()->stage->AVM1RemoveKeyboardListener(this);
	if (this->actions.AllEventFlags.ClipEventEnterFrame)
	{
		this->incRef();
		getSystemState()->unregisterFrameListener(_MR(this));
		getSystemState()->stage->AVM1RemoveEventListener(this);
	}
}
bool MovieClip::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	for (auto it = actions.ClipActionRecords.begin(); it != actions.ClipActionRecords.end(); it++)
	{
		if( (e->type == "keyDown" && it->EventFlags.ClipEventKeyDown) ||
				(e->type == "keyUp" && it->EventFlags.ClipEventKeyDown))
		{
			std::map<uint32_t,asAtom> m;
			ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,m);
		}
	}
	Sprite::AVM1HandleKeyboardEvent(e);
	return false;
}
bool MovieClip::AVM1HandleMouseEvent(EventDispatcher *dispatcher, MouseEvent *e)
{
	if (!this->isOnStage() || !this->enabled || !this->visible)
		return false;
	if (dispatcher->is<DisplayObject>())
	{
		number_t x,y,xg,yg;
		dispatcher->as<DisplayObject>()->localToGlobal(e->localX,e->localY,xg,yg);
		this->globalToLocal(xg,yg,x,y);
		_NR<DisplayObject> dispobj=hitTest(NullRef,x,y, DisplayObject::MOUSE_CLICK,true);
		if (!dispobj && ((e->type == "click")|| (e->type == "releaseOutside")))
				return false;
		for (auto it = actions.ClipActionRecords.begin(); it != actions.ClipActionRecords.end(); it++)
		{
			if( (e->type == "mouseDown" && it->EventFlags.ClipEventMouseDown)
					|| (e->type == "mouseUp" && it->EventFlags.ClipEventMouseUp)
					|| (e->type == "click" && it->EventFlags.ClipEventRelease)
					|| (e->type == "mouseDown" && it->EventFlags.ClipEventPress)
					|| (e->type == "mouseMove" && it->EventFlags.ClipEventMouseMove)
					|| (e->type == "rollOver" && it->EventFlags.ClipEventRollOver)
					|| (e->type == "rollOut" && it->EventFlags.ClipEventRollOut)
					|| (e->type == "releaseOutside" && it->EventFlags.ClipEventReleaseOutside)
					)
			{
				std::map<uint32_t,asAtom> m;
				ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,m);
			}
		}
		AVM1HandleMouseEventStandard(dispobj.getPtr(),e);
	}
	return false;
}
void MovieClip::AVM1HandleEvent(EventDispatcher *dispatcher, _R<Event> e)
{
	if (!this->isOnStage())
		return;
	std::map<uint32_t,asAtom> m;
	for (auto it = actions.ClipActionRecords.begin(); it != actions.ClipActionRecords.end(); it++)
	{
		if (e->type == "complete" && it->EventFlags.ClipEventLoad)
		{
			ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,m);
		}
		if (e->type == "enterFrame" && it->EventFlags.ClipEventEnterFrame)
		{
			if (!this->state.explicit_FP)
				ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,m);
		}
		if (e->type == "load" && it->EventFlags.ClipEventLoad)
		{
			ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,m);
		}
	}
	if (dispatcher == this)
	{
		if (e->type == "enterFrame")
		{
			if (!this->state.explicit_FP)
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
			
				m.name_s_id=BUILTIN_STRINGS::STRING_ONENTERFRAME;
				getVariableByMultiname(func,m);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				}
			}
		}
		else if (e->type == "load")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;

			m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
			getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			}
		}
	}
}


void MovieClip::setupActions(const CLIPACTIONS &clipactions)
{
	actions = clipactions;
	if (this->actions.AllEventFlags.ClipEventMouseDown ||
			this->actions.AllEventFlags.ClipEventMouseMove ||
			this->actions.AllEventFlags.ClipEventMouseUp)
	{
		setMouseEnabled(true);
		getSystemState()->stage->AVM1AddMouseListener(this);
	}
	if (this->actions.AllEventFlags.ClipEventKeyDown ||
			this->actions.AllEventFlags.ClipEventKeyUp)
		getSystemState()->stage->AVM1AddKeyboardListener(this);
	if (this->actions.AllEventFlags.ClipEventLoad)
		getSystemState()->stage->AVM1AddEventListener(this);
	if (this->actions.AllEventFlags.ClipEventEnterFrame)
	{
		this->incRef();
		getSystemState()->registerFrameListener(_MR(this));
		getSystemState()->stage->AVM1AddEventListener(this);
	}
}

MovieClip *MovieClip::AVM1GetClipFromPath(tiny_string &path)
{
	if (path.empty() || path == "this")
		return this;
	if (path =="_root")
	{
		return getSystemState()->mainClip;
	}
	if (path.startsWith("/"))
	{
		tiny_string newpath = path.substr_bytes(1,path.numBytes()-1);
		MovieClip* root = getSystemState()->mainClip;
		if (root)
			return root->AVM1GetClipFromPath(newpath);
		LOG(LOG_ERROR,"AVM1: no root movie clip for path:"<<path<<" "<<this->toDebugString());
		return nullptr;
	}
	if (path.startsWith("../"))
	{
		tiny_string newpath = path.substr_bytes(3,path.numBytes()-3);
		if (this->getParent() && this->getParent()->is<MovieClip>())
			return this->getParent()->as<MovieClip>()->AVM1GetClipFromPath(newpath);
		LOG(LOG_ERROR,"AVM1: no parent clip for path:"<<path<<" "<<this->toDebugString());
		return nullptr;
	}
	uint32_t pos = path.find("/");
	tiny_string subpath = (pos == tiny_string::npos) ? path : path.substr_bytes(0,pos);
	if (subpath.empty())
	{
		return nullptr;
	}
	// path "/stage" is mapped to the root movie (?) 
	if (this == getSystemState()->mainClip && subpath == "stage")
		return this;
	uint32_t posdot = subpath.find(".");
	if (posdot != tiny_string::npos)
	{
		tiny_string subdotpath =  subpath.substr_bytes(0,posdot);
		if (subdotpath.empty())
			return nullptr;
		MovieClip* parent = AVM1GetClipFromPath(subdotpath);
		if (!parent)
			return nullptr;
		tiny_string localname = subpath.substr_bytes(posdot+1,subpath.numBytes()-posdot-1);
		return parent->AVM1GetClipFromPath(localname);
	}
	
	multiname objName(NULL);
	objName.name_type=multiname::NAME_STRING;
	objName.name_s_id=getSystemState()->getUniqueStringId(subpath);
	objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	asAtom ret=asAtomHandler::invalidAtom;
	getVariableByMultiname(ret,objName);
	if (asAtomHandler::is<MovieClip>(ret))
	{
		if (pos == tiny_string::npos)
			return asAtomHandler::as<MovieClip>(ret);
		else
		{
			subpath = path.substr_bytes(pos+1,path.numBytes()-pos-1);
			return asAtomHandler::as<MovieClip>(ret)->AVM1GetClipFromPath(subpath);
		}
	}
	LOG(LOG_ERROR,"AVM1:"<<getTagID()<<" "<<state.FP<<" path not found:"<<path<<" "<<subpath<<" "<<asAtomHandler::toDebugString(ret));
	return nullptr;
}

void MovieClip::AVM1SetVariable(tiny_string &name, asAtom v)
{
	if (name.empty())
		return;
	if (name.startsWith("/"))
	{
		tiny_string newpath = name.substr_bytes(1,name.numBytes()-1);
		MovieClip* root = getSystemState()->mainClip;
		if (root)
			root->AVM1SetVariable(newpath,v);
		else
			LOG(LOG_ERROR,"AVM1: no root movie clip for name:"<<name<<" "<<this->toDebugString());
		return;
	}
	uint32_t pos = name.find(":");
	if (pos == tiny_string::npos)
	{
		tiny_string localname = name.lowercase();
		uint32_t nameId = getSystemState()->getUniqueStringId(localname);
		if (asAtomHandler::isUndefined(v))
			avm1variables.erase(nameId);
		else
			avm1variables[nameId] = v;
		multiname objName(NULL);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=nameId;
		setVariableByMultiname(objName,v, ASObject::CONST_ALLOWED);
		AVM1UpdateVariableBindings(nameId,v);
	}
	else if (pos == 0)
	{
		tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1).lowercase();
		uint32_t nameId = getSystemState()->getUniqueStringId(localname);
		if (asAtomHandler::isUndefined(v))
			avm1variables.erase(nameId);
		else
			avm1variables[nameId] = v;
	}
	else
	{
		tiny_string path = name.substr_bytes(0,pos);
		MovieClip* clip = AVM1GetClipFromPath(path);
		if (clip)
		{
			tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
			clip->AVM1SetVariable(localname,v);
		}
	}
}

asAtom MovieClip::AVM1GetVariable(const tiny_string &name)
{
	uint32_t pos = name.find(":");
	if (pos == tiny_string::npos)
	{
		auto it = avm1variables.find(getSystemState()->getUniqueStringId(name.lowercase()));
		if (it != avm1variables.end())
			return it->second;
	}
	else if (pos == 0)
	{
		tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
		return AVM1GetVariable(localname.lowercase());
	}
	else
	{
		tiny_string path = name.substr_bytes(0,pos).lowercase();
		MovieClip* clip = AVM1GetClipFromPath(path);
		if (clip)
		{
			tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
			return clip->AVM1GetVariable(localname.lowercase());
		}
	}
	asAtom ret=asAtomHandler::invalidAtom;
	
	if (getSystemState()->mainClip->version > 4)
	{
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=getSystemState()->getUniqueStringId(name);
		m.isAttribute = false;
		getVariableByMultiname(ret,m);
		if (asAtomHandler::isInvalid(ret))// get Variable from root movie
			getSystemState()->mainClip->getVariableByMultiname(ret,m);
		if (asAtomHandler::isInvalid(ret))// get Variable from global object
			getSystemState()->avm1global->getVariableByMultiname(ret,m);
	}
	return ret;
}
asAtom MovieClip::getVariableBindingValue(const tiny_string &name)
{
	uint32_t pos = name.find(".");
	asAtom ret=asAtomHandler::invalidAtom;
	if (pos == tiny_string::npos)
	{
		ret = AVM1GetVariable(name);
	}
	else
	{
		tiny_string firstpart = name.substr_bytes(0,pos);
		asAtom obj = AVM1GetVariable(firstpart);
		tiny_string localname = name.substr_bytes(pos+1,name.numBytes()-pos-1);
		ret = asAtomHandler::toObject(obj,getSystemState())->getVariableBindingValue(localname);
	}
	return ret;
}
void MovieClip::setVariableBinding(tiny_string &name, _NR<DisplayObject> obj)
{
	uint32_t key = getSystemState()->getUniqueStringId(name);
	if (obj)
	{
		obj->incRef();
		auto it = variablebindings.lower_bound(key);
		while (it != variablebindings.end() && it->first == key)
		{
			if (it->second == obj)
				return;
			it++;
		}
		variablebindings.insert(std::make_pair(key,obj));
	}
	else
	{
		auto it = variablebindings.find(key);
		while (it != variablebindings.end() && it->first == key)
		{
			if (it->second == obj)
			{
				variablebindings.erase(it);
				break;
			}
			it++;
		}
	}
}
void MovieClip::AVM1SetFunction(uint32_t nameID, _NR<AVM1Function> obj)
{
	if (obj)
	{
		obj->incRef();
		avm1functions[nameID] = obj;
		avm1variables[nameID] = asAtomHandler::fromObject(obj.getPtr());
	}
	else
	{
		avm1functions.erase(nameID);
		avm1variables.erase(nameID);
	}
}
AVM1Function* MovieClip::AVM1GetFunction(uint32_t nameID)
{
	auto it = avm1functions.find(nameID);
	if (it != avm1functions.end())
		return it->second.getPtr();
	return nullptr;
}
void MovieClip::AVM1SetupMethods(Class_base* c)
{
	DisplayObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("attachMovie","",Class<IFunction>::getFunction(c->getSystemState(),AVM1AttachMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),AVM1CreateEmptyMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),AVM1RemoveMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),AVM1Clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",Class<IFunction>::getFunction(c->getSystemState(),AVM1MoveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",Class<IFunction>::getFunction(c->getSystemState(),AVM1LineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",Class<IFunction>::getFunction(c->getSystemState(),AVM1LineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",Class<IFunction>::getFunction(c->getSystemState(),AVM1BeginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("endFill","",Class<IFunction>::getFunction(c->getSystemState(),AVM1EndFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),Sprite::_getter_useHandCursor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),Sprite::_setter_useHandCursor),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getNextHighestDepth","",Class<IFunction>::getFunction(c->getSystemState(),AVM1GetNextHighestDepth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachBitmap","",Class<IFunction>::getFunction(c->getSystemState(),AVM1AttachBitmap),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
}

void MovieClip::AVM1ExecuteFrameActionsFromLabel(const tiny_string &label)
{
	uint32_t dest=getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_INFO, "AVM1ExecuteFrameActionsFromLabel: label not found:" <<label);
		return;
	}
	AVM1ExecuteFrameActions(dest);
}

void MovieClip::AVM1ExecuteFrameActions(uint32_t frame)
{
	auto it=frames.begin();
	uint32_t i=0;
	while(it != frames.end() && i < frame)
	{
		++i;
		++it;
	}
	if (it != frames.end())
	{
		bool initactionsdone = frameinitactionsdone.find(frame) != frameinitactionsdone.end();
		it->AVM1executeActions(this,initactionsdone);
		if (!initactionsdone)
			frameinitactionsdone.insert(frame);
	}
}

void MovieClip::AVM1UpdateVariableBindings(uint32_t nameID, asAtom& value)
{
	auto it = variablebindings.find(nameID);
	while (it != variablebindings.end() && it->first == nameID)
	{
		(*it).second->UpdateVariableBinding(value);
		it++;
	}
}

ASFUNCTIONBODY_ATOM(MovieClip,AVM1AttachMovie)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen != 3 && argslen != 4)
		throw RunTimeException("AVM1: invalid number of arguments for attachMovie");
	int Depth = asAtomHandler::toInt(args[2]);
	uint32_t nameId = asAtomHandler::toStringId(args[1],sys);
	DictionaryTag* placedTag = sys->mainClip->dictionaryLookupByName(asAtomHandler::toStringId(args[0],sys));
	ASObject *instance = placedTag->instance();
	DisplayObject* toAdd=dynamic_cast<DisplayObject*>(instance);
	if(!toAdd && instance)
	{
		LOG(LOG_NOT_IMPLEMENTED, "AVM1: attachMovie adding non-DisplayObject to display list:"<<instance->toDebugString());
		return;
	}
	toAdd->name = nameId;
	if (argslen == 4)
	{
		ASObject* o = asAtomHandler::getObject(args[3]);
		if (o)
		{
			o->copyValues(toAdd);
		}
	}
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth);
		th->insertLegacyChildAt(Depth,toAdd);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd);
	toAdd->constructionComplete();
	toAdd->afterConstruction();
	ret=asAtomHandler::fromObject(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1CreateEmptyMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen != 2)
		throw RunTimeException("AVM1: invalid number of arguments for CreateEmptyMovieClip");
	int Depth = asAtomHandler::toInt(args[1]);
	uint32_t nameId = asAtomHandler::toStringId(args[0],sys);
	AVM1MovieClip* toAdd= Class<AVM1MovieClip>::getInstanceSNoArgs(sys);
	toAdd->name = nameId;
	toAdd->setMouseEnabled(false);
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth);
		th->insertLegacyChildAt(Depth,toAdd);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd);
	toAdd->constructionComplete();
	ret=asAtomHandler::fromObject(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1RemoveMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (th->getParent())
		th->getParent()->_removeChild(th);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1Clear)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::clear(ret,sys,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1MoveTo)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::moveTo(ret,sys,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1LineTo)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::lineTo(ret,sys,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1LineStyle)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::lineStyle(ret,sys,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1BeginFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	if(argslen>=2)
		args[1]=asAtomHandler::fromNumber(sys,asAtomHandler::toNumber(args[1])/100.0,false);
	
	Graphics::beginFill(ret,sys,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1EndFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::endFill(ret,sys,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1GetNextHighestDepth)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,sys,th->getMaxLegacyChildDepth()+1);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1AttachBitmap)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen < 2)
		throw RunTimeException("AVM1: invalid number of arguments for attachBitmap");
	int Depth = asAtomHandler::toInt(args[1]);
	if (!asAtomHandler::is<BitmapData>(args[0]))
	{
		LOG(LOG_ERROR,"AVM1AttachBitmap invalid type:"<<asAtomHandler::toDebugString(args[0]));
		throw RunTimeException("AVM1: attachBitmap first parameter is no BitmapData");
	}

	BitmapData* data = asAtomHandler::as<BitmapData>(args[0]);
	data->incRef();
	Bitmap* toAdd = Class<Bitmap>::getInstanceS(sys,_MR(data));
	if (argslen > 2)
		toAdd->pixelSnapping = asAtomHandler::toString(args[2],sys);
	if (argslen > 3)
		toAdd->smoothing = asAtomHandler::Boolean_concrete(args[3]);
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth);
		th->insertLegacyChildAt(Depth,toAdd);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd);
	ret=asAtomHandler::fromObject(toAdd);
}
void DisplayObjectContainer::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("numChildren","",Class<IFunction>::getFunction(c->getSystemState(),_getNumChildren),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("getChildIndex","",Class<IFunction>::getFunction(c->getSystemState(),_getChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setChildIndex","",Class<IFunction>::getFunction(c->getSystemState(),_setChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildAt","",Class<IFunction>::getFunction(c->getSystemState(),getChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildByName","",Class<IFunction>::getFunction(c->getSystemState(),getChildByName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getObjectsUnderPoint","",Class<IFunction>::getFunction(c->getSystemState(),getObjectsUnderPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChild","",Class<IFunction>::getFunction(c->getSystemState(),addChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChild","",Class<IFunction>::getFunction(c->getSystemState(),removeChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildAt","",Class<IFunction>::getFunction(c->getSystemState(),removeChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildren","",Class<IFunction>::getFunction(c->getSystemState(),removeChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChildAt","",Class<IFunction>::getFunction(c->getSystemState(),addChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildren","",Class<IFunction>::getFunction(c->getSystemState(),swapChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildrenAt","",Class<IFunction>::getFunction(c->getSystemState(),swapChildrenAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",Class<IFunction>::getFunction(c->getSystemState(),contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(c->getSystemState(),_setMouseChildren),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseChildren),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, tabChildren);
}

ASFUNCTIONBODY_GETTER_SETTER(DisplayObjectContainer, tabChildren);

void DisplayObjectContainer::buildTraits(ASObject* o)
{
}

DisplayObjectContainer::DisplayObjectContainer(Class_base* c):InteractiveObject(c),mouseChildren(true),tabChildren(true)
{
	subtype=SUBTYPE_DISPLAYOBJECTCONTAINER;
}

bool DisplayObjectContainer::hasLegacyChildAt(int32_t depth)
{
	auto i = depthToLegacyChild.left.find(depth);
	return i != depthToLegacyChild.left.end();
}
DisplayObject* DisplayObjectContainer::getLegacyChildAt(int32_t depth)
{
	return depthToLegacyChild.left.at(depth);
}


void DisplayObjectContainer::setupClipActionsAt(int32_t depth,const CLIPACTIONS& actions)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"setupClipActionsAt: no child at depth "<<depth);
		return;
	}
	DisplayObject* o = getLegacyChildAt(depth);
	if (o->is<MovieClip>())
		o->as<MovieClip>()->setupActions(actions);
}

void DisplayObjectContainer::checkRatioForLegacyChildAt(int32_t depth,uint32_t ratio)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"checkRatioForLegacyChildAt: no child at that depth");
		return;
	}
	depthToLegacyChild.left.at(depth)->checkRatio(ratio);
	this->hasChanged=true;
}
void DisplayObjectContainer::checkColorTransformForLegacyChildAt(int32_t depth,const CXFORMWITHALPHA& colortransform)
{
	if(!hasLegacyChildAt(depth))
		return;
	DisplayObject* o = depthToLegacyChild.left.at(depth);
	if (o->colorTransform.isNull())
		o->colorTransform=_NR<ColorTransform>(Class<ColorTransform>::getInstanceS(getSystemState(),colortransform));
	else
		o->colorTransform->setProperties(colortransform);
	o->hasChanged=true;
	this->hasChanged=true;
	this->requestInvalidation(getSystemState());
}

void DisplayObjectContainer::deleteLegacyChildAt(int32_t depth)
{
	if(!hasLegacyChildAt(depth))
		return;
	DisplayObject* obj = depthToLegacyChild.left.at(depth);
	if(obj->name != BUILTIN_STRINGS::EMPTY)
	{
		// ensure obj is not deleted before _removeChild is called
		obj->incRef();
		//The variable is not deleted, but just set to null
		//This is a tested behavior
		multiname objName(NULL);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=obj->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		setVariableByMultiname(objName,asAtomHandler::nullAtom, ASObject::CONST_NOT_ALLOWED);
	}

	obj->incRef();
	//this also removes it from depthToLegacyChild
	bool ret = _removeChild(obj);
	assert_and_throw(ret);

	if(obj->name != BUILTIN_STRINGS::EMPTY)
			obj->decRef();
}

void DisplayObjectContainer::insertLegacyChildAt(int32_t depth, DisplayObject* obj)
{
	if(hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"insertLegacyChildAt: there is already one child at that depth");
		return;
	}
	
	uint32_t insertpos = 0;
	// find DisplayObject to insert obj after
	DisplayObject* preobj=nullptr;
	for (auto it = depthToLegacyChild.begin(); it != depthToLegacyChild.end();it++)
	{
		if (it->left > depth)
			break;
		preobj = it->right;
	}
	if (preobj)
	{
		preobj->incRef();
		auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),_MR(preobj));
		if(it!=dynamicDisplayList.end())
		{
			insertpos = it-dynamicDisplayList.begin()+1;
		}
	}
	
	_addChildAt(_MR(obj),insertpos);
	if(obj->name != BUILTIN_STRINGS::EMPTY)
	{
		obj->incRef();
		multiname objName(NULL);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=obj->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		asAtom v = asAtomHandler::fromObject(obj);
		setVariableByMultiname(objName,v,ASObject::CONST_NOT_ALLOWED);
	}

	depthToLegacyChild.insert(boost::bimap<int32_t,DisplayObject*>::value_type(depth,obj));
	obj->afterLegacyInsert();
}

void DisplayObjectContainer::transformLegacyChildAt(int32_t depth, const MATRIX& mat)
{
	if(!hasLegacyChildAt(depth))
		return;
	depthToLegacyChild.left.at(depth)->setLegacyMatrix(mat);
}

void DisplayObjectContainer::purgeLegacyChildren()
{
	auto i = depthToLegacyChild.begin();
	while( i != depthToLegacyChild.end() )
	{
		if (i->left < 0)
			legacyChildrenMarkedForDeletion.insert(i->left);
		i++;
	}
}
uint32_t DisplayObjectContainer::getMaxLegacyChildDepth()
{
	auto it = depthToLegacyChild.left.begin();
	int32_t max =0;
	while (it !=depthToLegacyChild.left.end())
	{
		if (it->first > max)
			max = it->first;
		it++;
	}
	return max;
}
void DisplayObjectContainer::checkClipDepth()
{
	DisplayObject* clipobj = NULL;
	int depth = 0;
	for (auto it=depthToLegacyChild.begin(); it != depthToLegacyChild.end(); it++)
	{
		DisplayObject* obj = it->right;
		depth = it->left;
		if (obj->ClipDepth)
		{
			if (clipobj)
				clipobj->hasChanged = false; // ensure clipobj is not rendered
			clipobj = obj;
		}
		else if (clipobj && clipobj->ClipDepth > depth)
		{
			clipobj->incRef();
			obj->setMask(_NR<DisplayObject>(clipobj));
		}
		else
			obj->setMask(NullRef);
	}
	if (clipobj)
		clipobj->hasChanged = false; // ensure clipobj is not rendered
}

bool DisplayObjectContainer::destruct()
{
	//Release every child
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
		(*it)->setParent(nullptr);
	dynamicDisplayList.clear();
	mouseChildren = true;
	tabChildren = true;
	legacyChildrenMarkedForDeletion.clear();
	depthToLegacyChild.left.clear();
	return InteractiveObject::destruct();
}

InteractiveObject::InteractiveObject(Class_base* c):DisplayObject(c),mouseEnabled(true),doubleClickEnabled(false),accessibilityImplementation(NullRef),contextMenu(NullRef),tabEnabled(false),tabIndex(-1)
{
	subtype=SUBTYPE_INTERACTIVE_OBJECT;
}

InteractiveObject::~InteractiveObject()
{
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_constructor)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	EventDispatcher::_constructor(ret,sys,obj,NULL,0);
	//Object registered very early are not supported this way (Stage for example)
	if(sys->getInputThread())
		sys->getInputThread()->addListener(th);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_setMouseEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	assert_and_throw(argslen==1);
	th->mouseEnabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_getMouseEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	asAtomHandler::setBool(ret,th->mouseEnabled);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_setDoubleClickEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	assert_and_throw(argslen==1);
	th->doubleClickEnabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_getDoubleClickEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	asAtomHandler::setBool(ret,th->doubleClickEnabled);
}

bool InteractiveObject::destruct()
{
	if(getSystemState()->getInputThread())
		getSystemState()->getInputThread()->removeListener(this);
	contextMenu.reset();
	mouseEnabled = true;
	doubleClickEnabled =false;
	accessibilityImplementation.reset();
	tabEnabled = false;
	tabIndex = -1;
	return DisplayObject::destruct();
}

void InteractiveObject::buildTraits(ASObject* o)
{
}

void InteractiveObject::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_setMouseEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseEnabled),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_setDoubleClickEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_getDoubleClickEnabled),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, accessibilityImplementation);
	REGISTER_GETTER_SETTER(c, contextMenu);
	REGISTER_GETTER_SETTER(c, tabEnabled);
	REGISTER_GETTER_SETTER(c, tabIndex);
	REGISTER_GETTER_SETTER(c, focusRect);
}

ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, accessibilityImplementation);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, contextMenu);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabEnabled);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabIndex);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, focusRect); // stub

void DisplayObjectContainer::dumpDisplayList(unsigned int level)
{
	tiny_string indent(std::string(2*level, ' '));
	std::vector<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		Vector2f pos = (*it)->getXY();
		LOG(LOG_INFO, indent << (*it)->toDebugString() <<
		    " (" << pos.x << "," << pos.y << ") " <<
		    (*it)->getNominalWidth() << "x" << (*it)->getNominalHeight() << " " <<
		    ((*it)->isVisible() ? "v" : "") <<
		    ((*it)->isMask() ? "m" : "") << " cd=" <<(*it)->ClipDepth<<" "<<
			"a=" << (*it)->clippedAlpha() <<" '"<<getSystemState()->getStringFromUniqueId((*it)->name)<<"' tag="<<(*it)->getTagID());

		if ((*it)->is<DisplayObjectContainer>())
		{
			(*it)->as<DisplayObjectContainer>()->dumpDisplayList(level+1);
		}
	}
	auto i = depthToLegacyChild.begin();
	while( i != depthToLegacyChild.end() )
	{
		LOG(LOG_INFO, indent << "legacy:"<<i->left <<" "<<i->right->toDebugString());
		i++;
	}
}

void DisplayObjectContainer::setOnStage(bool staged, bool force)
{
	if(staged!=onStage||force)
	{
		//Make a copy of display list, and release the mutex
		//before calling setOnStage
		std::vector<_R<DisplayObject>> displayListCopy;
		{
			Locker l(mutexDisplayList);
			displayListCopy.assign(dynamicDisplayList.begin(),
					       dynamicDisplayList.end());
		}
		DisplayObject::setOnStage(staged,force);
		//Notify children
		//calling DisplayObject::setOnStage may have changed the onStage state of the children, 
		//but the addedToStage/removedFromStage event must always be dispatched
		std::vector<_R<DisplayObject>>::const_iterator it=displayListCopy.begin();
		for(;it!=displayListCopy.end();++it)
			(*it)->setOnStage(staged,true);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_constructor)
{
	InteractiveObject::_constructor(ret,sys,obj,NULL,0);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getNumChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	asAtomHandler::setInt(ret,sys,(int32_t)th->dynamicDisplayList.size());
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getMouseChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	asAtomHandler::setBool(ret,th->mouseChildren);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_setMouseChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	th->mouseChildren=asAtomHandler::Boolean_concrete(args[0]);
}

void DisplayObjectContainer::requestInvalidation(InvalidateQueue* q)
{
	DisplayObject::requestInvalidation(q);
	Locker l(mutexDisplayList);
	std::vector<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		(*it)->hasChanged = true;
		(*it)->requestInvalidation(q);
	}
}

void DisplayObjectContainer::_addChildAt(_R<DisplayObject> child, unsigned int index)
{
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	if(child->getParent())
	{
		//Child already in this container
		if(child->getParent()==this)
			return;
		else
		{
			child->incRef();
			child->getParent()->_removeChild(child.getPtr());
		}
	}
	child->setParent(this);
	{
		Locker l(mutexDisplayList);
		//We insert the object in the back of the list
		if(index >= dynamicDisplayList.size())
			dynamicDisplayList.push_back(child);
		else
		{
			std::vector<_R<DisplayObject>>::iterator it=dynamicDisplayList.begin();
			for(unsigned int i=0;i<index;i++)
				++it;
			dynamicDisplayList.insert(it,child);
		}
	}
	child->setOnStage(onStage);
}

bool DisplayObjectContainer::_removeChild(DisplayObject* child)
{
	if(!child->getParent() || child->getParent()!=this)
		return false;

	{
		Locker l(mutexDisplayList);
		std::vector<_R<DisplayObject>>::iterator it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),_MR(child));
		if(it==dynamicDisplayList.end())
			return false;

		child->setOnStage(false);
		child->setParent(nullptr);
		child->setMask(NullRef);
		
		//Erase this from the legacy child map (if it is in there)
		depthToLegacyChild.right.erase(child);

		dynamicDisplayList.erase(it);
	}
	return true;
}

bool DisplayObjectContainer::_contains(_R<DisplayObject> d)
{
	if(d==this)
		return true;

	std::vector<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		if(*it==d)
			return true;
		if((*it)->is<DisplayObjectContainer>() && (*it)->as<DisplayObjectContainer>()->_contains(d))
			return true;
	}
	return false;
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,contains)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(!asAtomHandler::is<DisplayObject>(args[0]))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	d->incRef();
	bool res=th->_contains(_MR(d));
	asAtomHandler::setBool(ret,res);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==2);
	if(asAtomHandler::isClass(args[0]) || asAtomHandler::isNull(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	int index=asAtomHandler::toInt(args[1]);

	//Cast to object
	ASATOM_INCREF(args[0]);
	_R<DisplayObject> d=_MR(asAtomHandler::as<DisplayObject>(args[0]));
	assert_and_throw(index >= 0 && (size_t)index<=th->dynamicDisplayList.size());
	th->_addChildAt(d,index);

	//Notify the object
	d->incRef();
	getVm(sys)->addEvent(d,_MR(Class<Event>::getInstanceS(sys,"added")));

	//incRef again as the value is getting returned
	d->incRef();
	ret = asAtomHandler::fromObject(d.getPtr());
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::isClass(args[0]) || asAtomHandler::isNull(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	//Cast to object
	ASATOM_INCREF(args[0]);
	_R<DisplayObject> d=_MR(asAtomHandler::as<DisplayObject>(args[0]));
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	d->incRef();
	getVm(sys)->addEvent(d,_MR(Class<Event>::getInstanceS(sys,"added")));

	d->incRef();
	ret = asAtomHandler::fromObject(d.getPtr());
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChild)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(!asAtomHandler::is<DisplayObject>(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	d->incRef();
	if(!th->_removeChild(d))
		throw Class<ArgumentError>::getInstanceS(sys,"removeChild: child not in list", 2025);

	//As we return the child we have to incRef it
	d->incRef();
	ret = asAtomHandler::fromObject(d);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	int32_t index=asAtomHandler::toInt(args[0]);

	DisplayObject* child;
	{
		Locker l(th->mutexDisplayList);
		if(index>=int(th->dynamicDisplayList.size()) || index<0)
			throw Class<RangeError>::getInstanceS(sys,"removeChildAt: invalid index", 2025);
		std::vector<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
		for(int32_t i=0;i<index;i++)
			++it;
		child=(*it).getPtr();
		//Erase this from the legacy child map (if it is in there)
		th->depthToLegacyChild.right.erase(child);
		child->setOnStage(false);
		child->setParent(nullptr);
		//incRef before the refrence is destroyed
		child->incRef();
		th->dynamicDisplayList.erase(it);
	}
	//As we return the child we don't decRef it
	ret = asAtomHandler::fromObject(child);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChildren)
{
	uint32_t beginindex;
	uint32_t endindex;
	ARG_UNPACK_ATOM(beginindex,0)(endindex,0x7fffffff);
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	{
		Locker l(th->mutexDisplayList);
		if (endindex > th->dynamicDisplayList.size())
			endindex = (uint32_t)th->dynamicDisplayList.size();
		th->dynamicDisplayList.erase(th->dynamicDisplayList.begin()+beginindex,th->dynamicDisplayList.begin()+endindex);
	}
}
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_setChildIndex)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==2);

	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
	ASATOM_INCREF(args[0]);
	_R<DisplayObject> child = _MR(asAtomHandler::as<DisplayObject>(args[0]));

	int index=asAtomHandler::toInt(args[1]);
	int curIndex = th->getChildIndex(child);

	if(curIndex == index)
		return;

	Locker l(th->mutexDisplayList);

	child->incRef();
	th->dynamicDisplayList.erase(th->dynamicDisplayList.begin()+curIndex); //remove from old position

	std::vector<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
	int i = 0;
	for(;it != th->dynamicDisplayList.end(); ++it)
		if(i++ == index)
		{
			child->incRef();
			th->dynamicDisplayList.insert(it, child);
			return;
		}

	child->incRef();
	th->dynamicDisplayList.push_back(child);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,swapChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==2);
	
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[1]));

	if (asAtomHandler::getObject(args[0]) == asAtomHandler::getObject(args[1]))
	{
		// Must return, otherwise crashes trying to erase the
		// same object twice
		return;
	}

	//Cast to object
	ASATOM_INCREF(args[0]);
	_R<DisplayObject> child1=_MR(asAtomHandler::as<DisplayObject>(args[0]));
	ASATOM_INCREF(args[1]);
	_R<DisplayObject> child2=_MR(asAtomHandler::as<DisplayObject>(args[1]));

	{
		Locker l(th->mutexDisplayList);
		std::vector<_R<DisplayObject>>::iterator it1=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child1);
		std::vector<_R<DisplayObject>>::iterator it2=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child2);
		if(it1==th->dynamicDisplayList.end() || it2==th->dynamicDisplayList.end())
			throw Class<ArgumentError>::getInstanceS(sys,"Argument is not child of this object", 2025);

		std::iter_swap(it1, it2);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,swapChildrenAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	int index1;
	int index2;
	ARG_UNPACK_ATOM(index1)(index2);

	if (index1 < 0 || index1 > (int)th->dynamicDisplayList.size() ||
		index2 < 0 || index2 > (int)th->dynamicDisplayList.size())
		throwError<RangeError>(kParamRangeError);
	if (index1 == index2)
	{
		return;
	}

	{
		Locker l(th->mutexDisplayList);
		std::iter_swap(th->dynamicDisplayList.begin() + index1, th->dynamicDisplayList.begin() + index2);
	}
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getChildByName)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	uint32_t wantedName=asAtomHandler::toStringId(args[0],sys);
	std::vector<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
	ASObject* res=NULL;
	for(;it!=th->dynamicDisplayList.end();++it)
	{
		if((*it)->name==wantedName)
		{
			res=(*it).getPtr();
			break;
		}
	}
	if(res)
	{
		res->incRef();
		ret = asAtomHandler::fromObject(res);
	}
	else
		asAtomHandler::setUndefined(ret);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	unsigned int index=asAtomHandler::toInt(args[0]);
	if(index>=th->dynamicDisplayList.size())
		throw Class<RangeError>::getInstanceS(sys,"getChildAt: invalid index", 2025);
	std::vector<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		++it;

	(*it)->incRef();
	ret = asAtomHandler::fromObject((*it).getPtr());
}

int DisplayObjectContainer::getChildIndex(_R<DisplayObject> child)
{
	std::vector<_R<DisplayObject>>::const_iterator it = dynamicDisplayList.begin();
	int ret = 0;
	do
	{
		if(it == dynamicDisplayList.end())
			throw Class<ArgumentError>::getInstanceS(getSystemState(),"getChildIndex: child not in list", 2025);
		if(*it == child)
			break;
		ret++;
		++it;
	}
	while(1);
	return ret;
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getChildIndex)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	//Cast to object
	_R<DisplayObject> d= _MR(asAtomHandler::as<DisplayObject>(args[0]));
	d->incRef();

	asAtomHandler::setInt(ret,sys,th->getChildIndex(d));
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getObjectsUnderPoint)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	_NR<Point> point;
	ARG_UNPACK_ATOM(point);
	Array* res = Class<Array>::getInstanceSNoArgs(th->getSystemState());
	if (!point.isNull())
		th->getObjectsFromPoint(point.getPtr(),res);
	ret = asAtomHandler::fromObject(res);
}

void DisplayObjectContainer::getObjectsFromPoint(Point* point, Array *ar)
{
	number_t xmin,xmax,ymin,ymax;
	MATRIX m;
	{
		Locker l(mutexDisplayList);
		auto it = dynamicDisplayList.begin();
		while (it != dynamicDisplayList.end())
		{
			(*it)->incRef();
			if ((*it)->getBounds(xmin,xmax,ymin,ymax,m))
			{
				if (xmin <= point->getX() && xmax >= point->getX()
						&& ymin <= point->getY() && ymax >= point->getY())
						ar->push(asAtomHandler::fromObject((*it).getPtr()));
			}
			if ((*it)->is<DisplayObjectContainer>())
				(*it)->as<DisplayObjectContainer>()->getObjectsFromPoint(point,ar);
			it++;
		}

	}
}

Shape::Shape(Class_base* c):DisplayObject(c),TokenContainer(this, this->getSystemState()->shapeTokenMemory),graphics(NullRef)
{
}

Shape::Shape(Class_base* c, const tokensVector& tokens, float scaling):
	DisplayObject(c),TokenContainer(this, this->getSystemState()->shapeTokenMemory, tokens, scaling),graphics(NullRef)
{
}

void Shape::finalize()
{
	DisplayObject::finalize();
	graphics.reset();
}

void Shape::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(c->getSystemState(),_getGraphics),GETTER_METHOD,true);
}

void Shape::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Shape,_constructor)
{
	DisplayObject::_constructor(ret,sys,obj,NULL,0);
}

ASFUNCTIONBODY_ATOM(Shape,_getGraphics)
{
	Shape* th=asAtomHandler::as<Shape>(obj);
	if(th->graphics.isNull())
		th->graphics=_MR(Class<Graphics>::getInstanceS(sys,th));
	th->graphics->incRef();
	ret = asAtomHandler::fromObject(th->graphics.getPtr());
}

MorphShape::MorphShape(Class_base* c):DisplayObject(c),TokenContainer(this, this->getSystemState()->morphShapeTokenMemory),morphshapetag(NULL)
{
	scaling = 1.0f/20.0f;
}

MorphShape::MorphShape(Class_base *c, DefineMorphShapeTag* _morphshapetag):DisplayObject(c),TokenContainer(this, this->getSystemState()->morphShapeTokenMemory),morphshapetag(_morphshapetag)
{
	scaling = 1.0f/20.0f;
}

void MorphShape::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, DisplayObject, CLASS_SEALED | CLASS_FINAL);
}

void MorphShape::buildTraits(ASObject* o)
{
	//No traits
}

void MorphShape::checkRatio(uint32_t ratio)
{
	TokenContainer::FromDefineMorphShapeTagToShapeVector(getSystemState(),this->morphshapetag,tokens,ratio);
	this->hasChanged = true;
	if (isOnStage())
		requestInvalidation(getSystemState());
}


void Stage::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("allowFullScreen","",Class<IFunction>::getFunction(c->getSystemState(),_getAllowFullScreen),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("allowFullScreenInteractive","",Class<IFunction>::getFunction(c->getSystemState(),_getAllowFullScreenInteractive),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("colorCorrectionSupport","",Class<IFunction>::getFunction(c->getSystemState(),_getColorCorrectionSupport),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",Class<IFunction>::getFunction(c->getSystemState(),_setStageWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",Class<IFunction>::getFunction(c->getSystemState(),_setStageHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageVideos","",Class<IFunction>::getFunction(c->getSystemState(),_getStageVideos),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",Class<IFunction>::getFunction(c->getSystemState(),_getFocus),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",Class<IFunction>::getFunction(c->getSystemState(),_setFocus),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",Class<IFunction>::getFunction(c->getSystemState(),_getFrameRate),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",Class<IFunction>::getFunction(c->getSystemState(),_setFrameRate),SETTER_METHOD,true);
	// override the setter from DisplayObjectContainer
	c->setDeclaredMethodByQName("tabChildren","",Class<IFunction>::getFunction(c->getSystemState(),_setTabChildren),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wmodeGPU","",Class<IFunction>::getFunction(c->getSystemState(),_getWmodeGPU),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("invalidate","",Class<IFunction>::getFunction(c->getSystemState(),_invalidate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),_getColor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),_setColor),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,align);
	REGISTER_GETTER_SETTER(c,colorCorrection);
	REGISTER_GETTER_SETTER(c,displayState);
	REGISTER_GETTER_SETTER(c,fullScreenSourceRect);
	REGISTER_GETTER_SETTER(c,showDefaultContextMenu);
	REGISTER_GETTER_SETTER(c,quality);
	REGISTER_GETTER_SETTER(c,stageFocusRect);
	REGISTER_GETTER(c,allowsFullScreen);
	REGISTER_GETTER(c,stage3Ds);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,align,onAlign);
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,colorCorrection,onColorCorrection);
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,displayState,onDisplayState);
ASFUNCTIONBODY_GETTER_SETTER(Stage,showDefaultContextMenu);  // stub
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,fullScreenSourceRect,onFullScreenSourceRect);
ASFUNCTIONBODY_GETTER_SETTER(Stage,quality);
ASFUNCTIONBODY_GETTER_SETTER(Stage,stageFocusRect);  // stub
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,allowsFullScreen);  // stub
ASFUNCTIONBODY_GETTER(Stage,stage3Ds); 

void Stage::onDisplayState(const tiny_string&)
{
	if (displayState != "normal")
		LOG(LOG_NOT_IMPLEMENTED,"Stage.displayState = " << displayState);
	displayState = "normal"; // until fullscreen support is implemented
}

void Stage::onAlign(const tiny_string& /*oldValue*/)
{
	LOG(LOG_NOT_IMPLEMENTED, "Stage.align = " << align);
}

void Stage::onColorCorrection(const tiny_string& oldValue)
{
	if (colorCorrection != "default" && 
	    colorCorrection != "on" && 
	    colorCorrection != "off")
	{
		colorCorrection = oldValue;
		throwError<ArgumentError>(kInvalidEnumError, "colorCorrection");
	}
}

void Stage::onFullScreenSourceRect(_NR<Rectangle> /*oldValue*/)
{
	LOG(LOG_NOT_IMPLEMENTED, "Stage.fullScreenSourceRect");
	fullScreenSourceRect.reset();
}

void Stage::eventListenerAdded(const tiny_string& eventName)
{
	if (eventName == "stageVideoAvailability")
	{
		// StageVideoAvailabilityEvent is dispatched directly after an eventListener is added added
		// see https://www.adobe.com/devnet/flashplayer/articles/stage_video.html 
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<StageVideoAvailabilityEvent>::getInstanceS(getSystemState())));
	}
}

void Stage::renderImpl(RenderContext &ctxt) const
{
	bool has3d = false;
	for (uint32_t i = 0; i < stage3Ds->size(); i++)
	{
		asAtom a=stage3Ds->at(i);
		if (asAtomHandler::as<Stage3D>(a)->renderImpl(ctxt))
			has3d = true;
	}
	if (has3d)
	{
		// setup opengl state for additional 2d rendering
		getSystemState()->getEngineData()->exec_glActiveTexture_GL_TEXTURE0(0);
		getSystemState()->getEngineData()->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
		getSystemState()->getEngineData()->exec_glUseProgram(((RenderThread&)ctxt).gpu_program);
		((GLRenderContext&)ctxt).lsglLoadIdentity();
		((GLRenderContext&)ctxt).setMatrixUniform(GLRenderContext::LSGL_MODELVIEW);
	}
	DisplayObjectContainer::renderImpl(ctxt);
}

void Stage::buildTraits(ASObject* o)
{
}

Stage::Stage(Class_base* c):
	DisplayObjectContainer(c), colorCorrection("default"),showDefaultContextMenu(true),quality("high"),stageFocusRect(false),allowsFullScreen(false)
{
	subtype = SUBTYPE_STAGE;
	onStage = true;
	asAtom v=asAtomHandler::invalidAtom;
	Template<Vector>::getInstanceS(v,getSystemState(),Class<Stage3D>::getClass(getSystemState()),NullRef);
	stage3Ds = _R<Vector>(asAtomHandler::as<Vector>(v));
	// according to specs, Desktop computers usually have 4 Stage3D objects available
	v =asAtomHandler::fromObject(Class<Stage3D>::getInstanceS(c->getSystemState()));
	stage3Ds->append(v);
	v =asAtomHandler::fromObject(Class<Stage3D>::getInstanceS(c->getSystemState()));
	stage3Ds->append(v);
	v =asAtomHandler::fromObject(Class<Stage3D>::getInstanceS(c->getSystemState()));
	stage3Ds->append(v);
	v =asAtomHandler::fromObject(Class<Stage3D>::getInstanceS(c->getSystemState()));
	stage3Ds->append(v);
}

_NR<Stage> Stage::getStage()
{
	this->incRef();
	return _MR(this);
}

ASFUNCTIONBODY_ATOM(Stage,_constructor)
{
}

_NR<DisplayObject> Stage::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret;
	ret = DisplayObjectContainer::hitTestImpl(last, x, y, type, interactiveObjectsOnly);
	if(!ret)
	{
		/* If nothing else is hit, we hit the stage */
		this->incRef();
		ret = _MNR(this);
	}
	return ret;
}

_NR<RootMovieClip> Stage::getRoot()
{
	return root;
}

void Stage::setRoot(_NR<RootMovieClip> _root)
{
	root = _root;
}

uint32_t Stage::internalGetWidth() const
{
	uint32_t width;
	if(getSystemState()->scaleMode==SystemState::NO_SCALE)
		width=getSystemState()->getRenderThread()->windowWidth;
	else
	{
		RECT size=getSystemState()->mainClip->getFrameSize();
		width=size.Xmax/20;
	}
	return width;
}

uint32_t Stage::internalGetHeight() const
{
	uint32_t height;
	if(getSystemState()->scaleMode==SystemState::NO_SCALE)
		height=getSystemState()->getRenderThread()->windowHeight;
	else
	{
		RECT size=getSystemState()->mainClip->getFrameSize();
		height=size.Ymax/20;
	}
	return height;
}

ASFUNCTIONBODY_ATOM(Stage,_getStageWidth)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	asAtomHandler::setUInt(ret,sys,th->internalGetWidth());
}

ASFUNCTIONBODY_ATOM(Stage,_setStageWidth)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Stage.stageWidth setter");
}

ASFUNCTIONBODY_ATOM(Stage,_getStageHeight)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	asAtomHandler::setUInt(ret,sys,th->internalGetHeight());
}

ASFUNCTIONBODY_ATOM(Stage,_setStageHeight)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Stage.stageHeight setter");
}

ASFUNCTIONBODY_ATOM(Stage,_getLoaderInfo)
{
	asAtom a = asAtomHandler::fromObject(sys->mainClip);
	RootMovieClip::_getLoaderInfo(ret,sys,a,NULL,0);
}

ASFUNCTIONBODY_ATOM(Stage,_getScaleMode)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	switch(sys->scaleMode)
	{
		case SystemState::EXACT_FIT:
			ret = asAtomHandler::fromString(sys,"exactFit");
			return;
		case SystemState::SHOW_ALL:
			ret = asAtomHandler::fromString(sys,"showAll");
			return;
		case SystemState::NO_BORDER:
			ret = asAtomHandler::fromString(sys,"noBorder");
			return;
		case SystemState::NO_SCALE:
			ret = asAtomHandler::fromString(sys,"noScale");
			return;
	}
}

ASFUNCTIONBODY_ATOM(Stage,_setScaleMode)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	const tiny_string& arg0=asAtomHandler::toString(args[0],sys);
	SystemState::SCALE_MODE oldScaleMode = sys->scaleMode;
	if(arg0=="exactFit")
		sys->scaleMode=SystemState::EXACT_FIT;
	else if(arg0=="showAll")
		sys->scaleMode=SystemState::SHOW_ALL;
	else if(arg0=="noBorder")
		sys->scaleMode=SystemState::NO_BORDER;
	else if(arg0=="noScale")
		sys->scaleMode=SystemState::NO_SCALE;

	if (oldScaleMode != sys->scaleMode)
	{
		RenderThread* rt=sys->getRenderThread();
		rt->requestResize(rt->windowWidth, rt->windowHeight, true);
	}
}

ASFUNCTIONBODY_ATOM(Stage,_getStageVideos)
{
	LOG(LOG_NOT_IMPLEMENTED, "Accelerated rendering through StageVideo not implemented, SWF should fall back to Video");
	Template<Vector>::getInstanceS(ret,sys,Class<StageVideo>::getClass(sys),NullRef);
}

_NR<InteractiveObject> Stage::getFocusTarget()
{
	SpinlockLocker l(focusSpinlock);
	if (focus.isNull() || !focus->isOnStage() || !focus->isVisible())
	{
		incRef();
		return _MNR(this);
	}
	else
	{
		return focus;
	}
}

void Stage::setFocusTarget(_NR<InteractiveObject> f)
{
	SpinlockLocker l(focusSpinlock);
	if (focus)
		focus->lostFocus();
	focus = f;
	if (focus)
		focus->gotFocus();
}

void Stage::initFrame()
{
	DisplayObjectContainer::initFrame();
	auto it = hiddenobjects.begin();
	while (it != hiddenobjects.end())
	{
		(*it)->initFrame();
		it++;
	}
}

void Stage::executeFrameScript()
{
	DisplayObjectContainer::executeFrameScript();
	auto it = hiddenobjects.begin();
	while (it != hiddenobjects.end())
	{
		(*it)->executeFrameScript();
		it++;
	}
	// only execute first frame of hidden objects (?)
	hiddenobjects.clear();
}

void Stage::AVM1HandleEvent(EventDispatcher* dispatcher, _R<Event> e)
{
	if (e->is<KeyboardEvent>())
	{
		vector<_R<ASObject>> tmplisteners = avm1KeyboardListeners;
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			if ((*it)->AVM1HandleKeyboardEvent(e->as<KeyboardEvent>()))
				break;
			it++;
		}
	}
	else if (e->is<MouseEvent>())
	{
		vector<_R<ASObject>> tmplisteners = avm1MouseListeners;
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			if ((*it)->AVM1HandleMouseEvent(dispatcher, e->as<MouseEvent>()))
				break;
			it++;
		}
	}
	else 
	{
		vector<_R<DisplayObject>> tmplisteners = avm1EventListeners;
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			(*it)->AVM1HandleEvent(dispatcher, e);
			it++;
		}
	}
}

void Stage::AVM1AddKeyboardListener(ASObject *o)
{
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it).getPtr() == o)
			return;
	}
	o->incRef();
	avm1KeyboardListeners.push_back(_MR(o));
}

void Stage::AVM1RemoveKeyboardListener(ASObject *o)
{
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it).getPtr() == o)
		{
			avm1KeyboardListeners.erase(it);
			break;
		}
	}
}
void Stage::AVM1AddMouseListener(ASObject *o)
{
	for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
	{
		if ((*it).getPtr() == o)
			return;
	}
	o->incRef();
	avm1MouseListeners.push_back(_MR(o));
}

void Stage::AVM1RemoveMouseListener(ASObject *o)
{
	for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
	{
		if ((*it).getPtr() == o)
		{
			avm1MouseListeners.erase(it);
			break;
		}
	}
}
void Stage::AVM1AddEventListener(DisplayObject *o)
{
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it).getPtr() == o)
			return;
	}
	o->incRef();
	avm1EventListeners.push_back(_MR(o));
}
void Stage::AVM1RemoveEventListener(DisplayObject *o)
{
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it).getPtr() == o)
		{
			avm1EventListeners.erase(it);
			break;
		}
	}
}


ASFUNCTIONBODY_ATOM(Stage,_getFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<InteractiveObject> focus = th->getFocusTarget();
	if (focus.isNull())
	{
		return;
	}
	else
	{
		focus->incRef();
		ret = asAtomHandler::fromObject(focus.getPtr());
	}
}

ASFUNCTIONBODY_ATOM(Stage,_setFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<InteractiveObject> focus;
	ARG_UNPACK_ATOM(focus);
	th->setFocusTarget(focus);
}

ASFUNCTIONBODY_ATOM(Stage,_setTabChildren)
{
	// The specs says that Stage.tabChildren should throw
	// IllegalOperationError, but testing shows that instead of
	// throwing this simply ignores the value.
}

ASFUNCTIONBODY_ATOM(Stage,_getFrameRate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<RootMovieClip> root = th->getRoot();
	if (root.isNull())
		asAtomHandler::setNumber(ret,sys,sys->mainClip->getFrameRate());
	else
		asAtomHandler::setNumber(ret,sys,root->getFrameRate());
}

ASFUNCTIONBODY_ATOM(Stage,_setFrameRate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	number_t frameRate;
	ARG_UNPACK_ATOM(frameRate);
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		root->setFrameRate(frameRate);
}

ASFUNCTIONBODY_ATOM(Stage,_getAllowFullScreen)
{
	asAtomHandler::setBool(ret,false); // until fullscreen support is implemented
}

ASFUNCTIONBODY_ATOM(Stage,_getAllowFullScreenInteractive)
{
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(Stage,_getColorCorrectionSupport)
{
	asAtomHandler::setBool(ret,false); // until color correction is implemented
}

ASFUNCTIONBODY_ATOM(Stage,_getWmodeGPU)
{
	asAtomHandler::setBool(ret,false);
}
ASFUNCTIONBODY_ATOM(Stage,_invalidate)
{
	LOG(LOG_NOT_IMPLEMENTED,"invalidate not implemented yet");
	// TODO this crashes lightspark
	Stage* th=asAtomHandler::as<Stage>(obj);
	th->incRef();
	_R<FlushInvalidationQueueEvent> event=_MR(new (sys->unaccountedMemory) FlushInvalidationQueueEvent());
	getVm(sys)->addEvent(_MR(th),event);
}
ASFUNCTIONBODY_ATOM(Stage,_getColor)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	RGB rgb;
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		rgb = root->getBackground();
	asAtomHandler::setUInt(ret,sys,rgb.toUInt());
}

ASFUNCTIONBODY_ATOM(Stage,_setColor)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	uint32_t color;
	ARG_UNPACK_ATOM(color);
	RGB rgb(color);
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		root->setBackground(rgb);
}


void StageScaleMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("EXACT_FIT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"exactFit"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_BORDER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noBorder"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_SCALE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noScale"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHOW_ALL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"showAll"),CONSTANT_TRAIT);
}

void StageAlign::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BOTTOM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"B"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOTTOM_LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"BL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOTTOM_RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"BR"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"L"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"R"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"T"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP_LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"TL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP_RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"TR"),CONSTANT_TRAIT);
}

void StageQuality::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BEST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"best"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"high"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOW",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"low"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MEDIUM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"medium"),CONSTANT_TRAIT);
}

void StageDisplayState::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("FULL_SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreen"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FULL_SCREEN_INTERACTIVE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreenInteractive"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}

Bitmap::Bitmap(Class_base* c, _NR<LoaderInfo> li, std::istream *s, FILE_TYPE type):
	DisplayObject(c),TokenContainer(this, this->getSystemState()->bitmapTokenMemory),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	if(li)
	{
		loaderInfo = li;
		this->incRef();
		loaderInfo->setWaitedObject(_MR(this));
	}

	bitmapData = _MR(Class<BitmapData>::getInstanceS(c->getSystemState()));
	bitmapData->addUser(this);
	if(!s)
		return;

	if(type==FT_UNKNOWN)
	{
		// Try to detect the format from the stream
		UI8 Signature[4];
		(*s) >> Signature[0] >> Signature[1] >> Signature[2] >> Signature[3];
		type=ParseThread::recognizeFile(Signature[0], Signature[1],
						Signature[2], Signature[3]);
		s->putback(Signature[3]).putback(Signature[2]).
		   putback(Signature[1]).putback(Signature[0]);
	}

	switch(type)
	{
		case FT_JPEG:
			bitmapData->getBitmapContainer()->fromJPEG(*s);
			break;
		case FT_PNG:
			bitmapData->getBitmapContainer()->fromPNG(*s);
			break;
		case FT_GIF:
			LOG(LOG_NOT_IMPLEMENTED, _("GIFs are not yet supported"));
			break;
		default:
			LOG(LOG_ERROR,_("Unsupported image type"));
			break;
	}
	Bitmap::updatedData();
}

Bitmap::Bitmap(Class_base* c, _R<BitmapData> data) : DisplayObject(c),TokenContainer(this, this->getSystemState()->bitmapTokenMemory),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	bitmapData = data;
	bitmapData->addUser(this);
	Bitmap::updatedData();
}

Bitmap::~Bitmap()
{
}

bool Bitmap::destruct()
{
	if(!bitmapData.isNull())
		bitmapData->removeUser(this);
	bitmapData.reset();
	smoothing = false;
	return DisplayObject::destruct();
}

void Bitmap::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	REGISTER_GETTER_SETTER(c,bitmapData);
	REGISTER_GETTER_SETTER(c,smoothing);
	REGISTER_GETTER_SETTER(c,pixelSnapping);

}

ASFUNCTIONBODY_ATOM(Bitmap,_constructor)
{
	tiny_string _pixelSnapping;
	_NR<BitmapData> _bitmapData;
	Bitmap* th = asAtomHandler::as<Bitmap>(obj);
	ARG_UNPACK_ATOM(_bitmapData, NullRef)(_pixelSnapping, "auto")(th->smoothing, false);

	DisplayObject::_constructor(ret,sys,obj,NULL,0);

	if(_pixelSnapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap constructor doesn't support pixelSnapping:"<<_pixelSnapping);
	th->pixelSnapping = _pixelSnapping;

	if(!_bitmapData.isNull())
	{
		th->bitmapData=_bitmapData;
		th->bitmapData->addUser(th);
		th->updatedData();
	}
}

void Bitmap::onBitmapData(_NR<BitmapData> old)
{
	if(!old.isNull())
		old->removeUser(this);
	if(!bitmapData.isNull())
		bitmapData->addUser(this);
	Bitmap::updatedData();
}

void Bitmap::onSmoothingChanged(bool /*old*/)
{
	updatedData();
}

void Bitmap::onPixelSnappingChanged(tiny_string snapping)
{
	if(snapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap doesn't support pixelSnapping:"<<snapping);
	pixelSnapping = snapping;
}

ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,bitmapData,onBitmapData);
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,smoothing,onSmoothingChanged);
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,pixelSnapping,onPixelSnappingChanged);

void Bitmap::updatedData()
{
	tokens.clear();

	if(bitmapData.isNull() || bitmapData->getBitmapContainer().isNull())
		return;

	FILLSTYLE style(0xff);
	if (smoothing)
		style.FillStyleType=CLIPPED_BITMAP;
	else
		style.FillStyleType=NON_SMOOTHED_CLIPPED_BITMAP;
	style.bitmap=bitmapData->getBitmapContainer();
	tokens.filltokens.emplace_back(_MR(new GeomToken(SET_FILL, style)));
	tokens.filltokens.emplace_back(_MR(new GeomToken(MOVE, Vector2(0, 0))));
	tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(0, style.bitmap->getHeight()))));
	tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(style.bitmap->getWidth(), style.bitmap->getHeight()))));
	tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(style.bitmap->getWidth(), 0))));
	tokens.filltokens.emplace_back(_MR(new GeomToken(STRAIGHT, Vector2(0, 0))));
	hasChanged=true;
	if(onStage)
		requestInvalidation(getSystemState());
}
bool Bitmap::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);
}

_NR<DisplayObject> Bitmap::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	//Simple check inside the area, opacity data should not be considered
	//NOTE: on the X axis the 0th line must be ignored, while the one past the width is valid
	//NOTE: on the Y asix the 0th line is valid, while the one past the width is not
	//NOTE: This is tested behaviour!
	if(!bitmapData.isNull() && x > 0 && x <= bitmapData->getWidth() && y >=0 && y < bitmapData->getHeight())
	{
		if (interactiveObjectsOnly)
		{
			// when checking interactive objects only, we need the real object that is hitted, it will be properly handled in the parent
			this->incRef();
			return _MR(this);
		}
		return last;
	}
	return NullRef;
}

IntSize Bitmap::getBitmapSize() const
{
	if(bitmapData.isNull())
		return IntSize(0, 0);
	else
		return IntSize(bitmapData->getWidth(), bitmapData->getHeight());
}

void SimpleButton::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("upState","",Class<IFunction>::getFunction(c->getSystemState(),_getUpState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("upState","",Class<IFunction>::getFunction(c->getSystemState(),_setUpState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",Class<IFunction>::getFunction(c->getSystemState(),_getDownState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",Class<IFunction>::getFunction(c->getSystemState(),_setDownState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",Class<IFunction>::getFunction(c->getSystemState(),_getOverState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",Class<IFunction>::getFunction(c->getSystemState(),_setOverState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",Class<IFunction>::getFunction(c->getSystemState(),_getHitTestState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",Class<IFunction>::getFunction(c->getSystemState(),_setHitTestState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(c->getSystemState(),_getEnabled),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(c->getSystemState(),_setEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),_getUseHandCursor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),_setUseHandCursor),SETTER_METHOD,true);
}

void SimpleButton::buildTraits(ASObject* o)
{
}

void SimpleButton::afterLegacyInsert()
{
	getSystemState()->stage->AVM1AddKeyboardListener(this);
	getSystemState()->stage->AVM1AddMouseListener(this);
}

void SimpleButton::afterLegacyDelete(DisplayObjectContainer *par)
{
	getSystemState()->stage->AVM1RemoveKeyboardListener(this);
	getSystemState()->stage->AVM1RemoveMouseListener(this);
}

bool SimpleButton::AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e)
{
	if (!this->isOnStage() || !this->enabled || !this->isVisible())
		return false;
	if (!dispatcher->is<DisplayObject>())
		return false;
	_NR<DisplayObject> dispobj;
	if(e->type == "mouseOut")
	{
		if (dispatcher!= this)
			return false;
	}
	else
	{
		if (dispatcher == this)
			dispobj=hitTest(NullRef,e->localX,e->localY, DisplayObject::MOUSE_CLICK,true);
		else
		{
			number_t x,y;
			dispatcher->as<DisplayObject>()->localToGlobal(e->localX,e->localY,x,y);
			number_t x1,y1;
			this->globalToLocal(x,y,x1,y1);
			dispobj=hitTest(NullRef,x1,y1, DisplayObject::MOUSE_CLICK,true);
		}
		if (dispobj.getPtr()!= this)
			return false;
	}
	BUTTONSTATE oldstate = currentState;
	if(e->type == "mouseDown")
	{
		currentState = DOWN;
		reflectState();
	}
	else if(e->type == "mouseUp")
	{
		currentState = UP;
		reflectState();
	}
	else if(e->type == "mouseOver")
	{
		currentState = OVER;
		reflectState();
	}
	else if(e->type == "mouseOut")
	{
		currentState = STATE_OUT;
		reflectState();
	}
	bool handled = false;
	if (buttontag)
	{
		for (auto it = buttontag->condactions.begin(); it != buttontag->condactions.end(); it++)
		{
			if (  (it->CondIdleToOverDown && currentState==DOWN)
				||(it->CondOutDownToIdle && oldstate==DOWN && currentState==STATE_OUT)
				||(it->CondOutDownToOverDown && oldstate==DOWN && currentState==OVER)
				||(it->CondOverDownToOutDown && (oldstate==DOWN || oldstate==OVER) && currentState==STATE_OUT)
				||(it->CondOverDownToOverUp && (oldstate==DOWN || oldstate==OVER) && currentState==UP)
				||(it->CondOverUpToOverDown && (oldstate==UP || oldstate==OVER) && currentState==DOWN)
				||(it->CondOverUpToIdle && (oldstate==UP || oldstate==OVER) && currentState==STATE_OUT)
				||(it->CondIdleToOverUp && oldstate==STATE_OUT && currentState==OVER)
				||(it->CondOverDownToIdle && oldstate==DOWN && currentState==OVER)
				)
			{
				DisplayObjectContainer* c = getParent();
				while (c && !c->is<MovieClip>())
					c = c->getParent();
				if (c)
				{
					std::map<uint32_t,asAtom> m;
					ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->getCurrentFrame()->getAVM1Context(),it->actions,m);
					handled = true;
				}
				
			}
		}
	}
	handled |= AVM1HandleMouseEventStandard(dispobj.getPtr(),e);
	return handled;
}

bool SimpleButton::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	bool handled=false;
	for (auto it = this->buttontag->condactions.begin(); it != this->buttontag->condactions.end(); it++)
	{
		bool execute=false;
		uint32_t code = e->getCharCode();
		if (e->getModifiers() & KMOD_SHIFT)
		{
			switch (it->CondKeyPress)
			{
				case 33:// !
					execute = code==SDL_SCANCODE_1;break;
				case 34:// "
					execute = code==SDL_SCANCODE_APOSTROPHE;break;
				case 35:// #
					execute = code==SDL_SCANCODE_3;break;
				case 36:// $
					execute = code==SDL_SCANCODE_4;break;
				case 37:// %
					execute = code==SDL_SCANCODE_5;break;
				case 38:// &
					execute = code==SDL_SCANCODE_7;break;
				case 40:// (
					execute = code==SDL_SCANCODE_9;break;
				case 41:// )
					execute = code==SDL_SCANCODE_0;break;
				case 42:// *
					execute = code==SDL_SCANCODE_8;break;
				case 43:// +
					execute = code==SDL_SCANCODE_EQUALS;break;
				case 58:// :
					execute = code==SDL_SCANCODE_SEMICOLON;break;
				case 60:// <
					execute = code==SDL_SCANCODE_COMMA;break;
				case 62:// >
					execute = code==SDL_SCANCODE_PERIOD;break;
				case 63:// ?
					execute = code==SDL_SCANCODE_SLASH;break;
				case 64:// @
					execute = code==SDL_SCANCODE_2;break;
				case 94:// ^
					execute = code==SDL_SCANCODE_6;break;
				case 95:// _
					execute = code==SDL_SCANCODE_MINUS;break;
				case 123:// {
					execute = code==SDL_SCANCODE_LEFTBRACKET;break;
				case 124:// |
					execute = code==SDL_SCANCODE_BACKSLASH;break;
				case 125:// }
					execute = code==SDL_SCANCODE_RIGHTBRACKET;break;
				case 126:// ~
					execute = code==SDL_SCANCODE_GRAVE;break;
				default:// A-Z
					execute = it->CondKeyPress>=65
							&& it->CondKeyPress<=90
							&& code-SDL_SCANCODE_A==it->CondKeyPress-65;
					break;
			}
		}
		else
		{
			switch (it->CondKeyPress)
			{
				case 1:
					execute = code==SDL_SCANCODE_LEFT;break;
				case 2:
					execute = code==SDL_SCANCODE_RIGHT;break;
				case 3:
					execute = code==SDL_SCANCODE_HOME;break;
				case 4:
					execute = code==SDL_SCANCODE_END;break;
				case 5:
					execute = code==SDL_SCANCODE_INSERT;break;
				case 6:
					execute = code==SDL_SCANCODE_DELETE;break;
				case 8:
					execute = code==SDL_SCANCODE_BACKSPACE;break;
				case 13:
					execute = code==SDL_SCANCODE_RETURN;break;
				case 14:
					execute = code==SDL_SCANCODE_UP;break;
				case 15:
					execute = code==SDL_SCANCODE_DOWN;break;
				case 16:
					execute = code==SDL_SCANCODE_PAGEUP;break;
				case 17:
					execute = code==SDL_SCANCODE_PAGEDOWN;break;
				case 18:
					execute = code==SDL_SCANCODE_TAB;break;
				case 19:
					execute = code==SDL_SCANCODE_ESCAPE;break;
				case 32:
					execute = code==SDL_SCANCODE_SPACE;break;
				case 39:// '
					execute = code==SDL_SCANCODE_APOSTROPHE;break;
				case 44:// ,
					execute = code==SDL_SCANCODE_COMMA;break;
				case 45:// -
					execute = code==SDL_SCANCODE_MINUS;break;
				case 46:// .
					execute = code==SDL_SCANCODE_PERIOD;break;
				case 47:// /
					execute = code==SDL_SCANCODE_SLASH;break;
				case 48:// 0
					execute = code==SDL_SCANCODE_0;break;
				case 49:// 1
					execute = code==SDL_SCANCODE_1;break;
				case 50:// 2
					execute = code==SDL_SCANCODE_2;break;
				case 51:// 3
					execute = code==SDL_SCANCODE_3;break;
				case 52:// 4
					execute = code==SDL_SCANCODE_4;break;
				case 53:// 5
					execute = code==SDL_SCANCODE_5;break;
				case 54:// 6
					execute = code==SDL_SCANCODE_6;break;
				case 55:// 7
					execute = code==SDL_SCANCODE_7;break;
				case 56:// 8
					execute = code==SDL_SCANCODE_8;break;
				case 57:// 9
					execute = code==SDL_SCANCODE_9;break;
				case 59:// ;
					execute = code==SDL_SCANCODE_SEMICOLON;break;
				case 61:// =
					execute = code==SDL_SCANCODE_EQUALS;break;
				case 91:// [
					execute = code==SDL_SCANCODE_LEFTBRACKET;break;
				case 92:// 
					execute = code==SDL_SCANCODE_BACKSLASH;break;
				case 93:// ]
					execute = code==SDL_SCANCODE_RIGHTBRACKET;break;
				case 96:// `
					execute = code==SDL_SCANCODE_GRAVE;break;
				default:// a-z
					execute = it->CondKeyPress>=97
							&& it->CondKeyPress<=122
							&& code-SDL_SCANCODE_A==it->CondKeyPress-97;
					break;
			}
		}
		if (execute)
		{
			DisplayObjectContainer* c = getParent();
			while (c && !c->is<MovieClip>())
				c = c->getParent();
			std::map<uint32_t,asAtom> m;
			ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->getCurrentFrame()->getAVM1Context(),it->actions,m);
			handled=true;
		}
	}
	if (!handled)
		DisplayObjectContainer::AVM1HandleKeyboardEvent(e);
	return handled;
}


_NR<DisplayObject> SimpleButton::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;
	if(hitTestState)
	{
		this->incRef();
		ret = hitTestState->hitTest(_MR(this), x, y, type,false);
	}
	/* mouseDown events, for example, are never dispatched to the hitTestState,
	 * but directly to this button (and with event.target = this). This has been
	 * tested with the official flash player. It cannot work otherwise, as
	 * hitTestState->parent == NULL. (This has also been verified)
	 */
	if(ret)
	{
		if(!isHittable(type))
			return NullRef;

		this->incRef();
		ret = _MR(this);
	}
	return ret;
}

void SimpleButton::defaultEventBehavior(_R<Event> e)
{
	if(e->type == "mouseDown")
	{
		currentState = DOWN;
		reflectState();
	}
	else if(e->type == "mouseUp")
	{
		currentState = UP;
		reflectState();
	}
	else if(e->type == "mouseOver")
	{
		currentState = OVER;
		reflectState();
	}
	else if(e->type == "mouseOut")
	{
		currentState = STATE_OUT;
		reflectState();
	}
}

SimpleButton::SimpleButton(Class_base* c, DisplayObject *dS, DisplayObject *hTS,
				DisplayObject *oS, DisplayObject *uS, DefineButtonTag *tag)
	: DisplayObjectContainer(c), downState(dS), hitTestState(hTS), overState(oS), upState(uS),
	  buttontag(tag),currentState(STATE_OUT),enabled(true),useHandCursor(true)
{
	/* When called from DefineButton2Tag::instance, they are not constructed yet
	 * TODO: construct them here for once, or each time they become visible?
	 */
	if(dS) dS->initFrame();
	if(hTS) hTS->initFrame();
	if(oS) oS->initFrame();
	if(uS) uS->initFrame();

	tabEnabled = true;
}

void SimpleButton::finalize()
{
	DisplayObjectContainer::finalize();
	downState.reset();
	hitTestState.reset();
	overState.reset();
	upState.reset();
	buttontag=nullptr;
}

IDrawable *SimpleButton::invalidate(DisplayObject *target, const MATRIX &initialMatrix, bool smoothing)
{
	if (!upState.isNull())
		upState->invalidate(target,initialMatrix,smoothing);
	if (!overState.isNull())
		overState->invalidate(target,initialMatrix,smoothing);
	if (!downState.isNull())
		downState->invalidate(target,initialMatrix,smoothing);
	if (!hitTestState.isNull())
		hitTestState->invalidate(target,initialMatrix,smoothing);
	return DisplayObjectContainer::invalidate(target, initialMatrix,smoothing);
}
void SimpleButton::requestInvalidation(InvalidateQueue* q)
{
	if (!upState.isNull())
	{
		upState->hasChanged = true;
		if (upState->colorTransform.isNull())
			upState->colorTransform = this->colorTransform;
		upState->requestInvalidation(q);
	}
	if (!overState.isNull())
	{
		overState->hasChanged = true;
		if (overState->colorTransform.isNull())
			overState->colorTransform = this->colorTransform;
		overState->requestInvalidation(q);
	}
	if (!downState.isNull())
	{
		downState->hasChanged = true;
		if (downState->colorTransform.isNull())
			downState->colorTransform = this->colorTransform;
		downState->requestInvalidation(q);
	}
	if (!hitTestState.isNull())
	{
		hitTestState->hasChanged = true;
		if (hitTestState->colorTransform.isNull())
			hitTestState->colorTransform = this->colorTransform;
		hitTestState->requestInvalidation(q);
	}
	
	DisplayObjectContainer::requestInvalidation(q);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_constructor)
{
	/* This _must_ not call the DisplayObjectContainer
	 * see note at the class declaration.
	 */
	InteractiveObject::_constructor(ret,sys,obj,NULL,0);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	_NR<DisplayObject> upState;
	_NR<DisplayObject> overState;
	_NR<DisplayObject> downState;
	_NR<DisplayObject> hitTestState;
	ARG_UNPACK_ATOM(upState, NullRef)(overState, NullRef)(downState, NullRef)(hitTestState, NullRef);

	if (!upState.isNull())
		th->upState = upState;
	if (!overState.isNull())
		th->overState = overState;
	if (!downState.isNull())
		th->downState = downState;
	if (!hitTestState.isNull())
		th->hitTestState = hitTestState;

	th->reflectState();
}

void SimpleButton::reflectState()
{
	assert(dynamicDisplayList.empty() || dynamicDisplayList.size() == 1);
	if(!dynamicDisplayList.empty())
		_removeChild(dynamicDisplayList.front().getPtr());

	if((currentState == UP || currentState == STATE_OUT) && !upState.isNull())
	{
		upState->incRef();
		_addChildAt(upState,0);
	}
	else if(currentState == DOWN && !downState.isNull())
	{
		downState->incRef();
		_addChildAt(downState,0);
	}
	else if(currentState == OVER && !overState.isNull())
	{
		overState->incRef();
		_addChildAt(overState,0);
	}
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getUpState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->upState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->upState->incRef();
	ret = asAtomHandler::fromObject(th->upState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setUpState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->upState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->upState->incRef();
	th->reflectState();
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getHitTestState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->hitTestState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->hitTestState->incRef();
	ret = asAtomHandler::fromObject(th->hitTestState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setHitTestState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->hitTestState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->hitTestState->incRef();
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getOverState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->overState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->overState->incRef();
	ret = asAtomHandler::fromObject(th->overState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setOverState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->overState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->overState->incRef();
	th->reflectState();
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getDownState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->downState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->downState->incRef();
	ret = asAtomHandler::fromObject(th->downState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setDownState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->downState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->downState->incRef();
	th->reflectState();
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setEnabled)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	assert_and_throw(argslen==1);
	th->enabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getEnabled)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	asAtomHandler::setBool(ret,th->enabled);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setUseHandCursor)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	assert_and_throw(argslen==1);
	th->useHandCursor=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getUseHandCursor)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	asAtomHandler::setBool(ret,th->useHandCursor);
}

void GradientType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"linear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RADIAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"radial"),CONSTANT_TRAIT);
}

void BlendMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ADD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"add"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"alpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DARKEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"darken"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DIFFERENCE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"difference"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ERASE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"erase"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HARDLIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"hardlight"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVERT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invert"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LAYER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"layer"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LIGHTEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"lighten"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MULTIPLY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"multiply"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OVERLAY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"overlay"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"screen"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SUBTRACT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"subtract"),CONSTANT_TRAIT);
}

void SpreadMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("PAD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pad"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REFLECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"reflect"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REPEAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"repeat"),CONSTANT_TRAIT);
}

void InterpolationMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("RGB",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rgb"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINEAR_RGB",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"linearRGB"),CONSTANT_TRAIT);
}

void GraphicsPathCommand::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CUBIC_CURVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CURVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINE_TO",nsNameAndKind(),asAtomHandler::fromUInt(2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MOVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_OP",nsNameAndKind(),asAtomHandler::fromUInt(0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WIDE_LINE_TO",nsNameAndKind(),asAtomHandler::fromUInt(5),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WIDE_MOVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(4),CONSTANT_TRAIT);
}

void GraphicsPathWinding::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("EVEN_ODD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"evenOdd"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NON_ZERO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"nonZero"),CONSTANT_TRAIT);
}

void PixelSnapping::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALWAYS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"always"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"never"),CONSTANT_TRAIT);

}

void CapsStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"round"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQUARE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"square"),CONSTANT_TRAIT);
}

void JointStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BEVEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bevel"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MITER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"miter"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"round"),CONSTANT_TRAIT);
}

void DisplayObjectContainer::declareFrame()
{
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
		(*it)->declareFrame();
	DisplayObject::declareFrame();
}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void DisplayObjectContainer::initFrame()
{
	/* init the frames and call constructors of our children first */

	// elements of the dynamicDisplayList may be removed during initFrame() calls,
	// so we create a temporary list containing all elements
	std::vector < _R<DisplayObject> > tmplist;
	{
		Locker l(mutexDisplayList);
		tmplist.assign(dynamicDisplayList.begin(),dynamicDisplayList.end());
	}
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->initFrame();
	/* call our own constructor, if necassary */
	DisplayObject::initFrame();
}

void DisplayObjectContainer::executeFrameScript()
{
	// elements of the dynamicDisplayList may be removed during executeFrameScript() calls,
	// so we create a temporary list containing all elements
	std::vector < _R<DisplayObject> > tmplist;
	{
		Locker l(mutexDisplayList);
		tmplist.assign(dynamicDisplayList.begin(),dynamicDisplayList.end());
	}
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->executeFrameScript();
	DisplayObject::executeFrameScript();
}

multiname *DisplayObjectContainer::setVariableByMultiname(const multiname &name, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool *alreadyset)
{
	if (asAtomHandler::is<DisplayObject>(o))
	{
		// it seems that setting a new value for a named existing dynamic child removes the child from the display list
		variable* v = findVariableByMultiname(name,this->getClass());
		if (v && v->kind == TRAIT_KIND::DYNAMIC_TRAIT && asAtomHandler::is<DisplayObject>(v->var))
		{
			DisplayObject* obj = asAtomHandler::as<DisplayObject>(v->var);
			if (!obj->legacy)
			{
				obj->incRef();
				_removeChild(obj);
			}
		}
	}
	return InteractiveObject::setVariableByMultiname(name,o,allowConst,alreadyset);
}

bool DisplayObjectContainer::deleteVariableByMultiname(const multiname &name)
{
	variable* v = findVariableByMultiname(name,this->getClass());
	if (v && v->kind == TRAIT_KIND::DYNAMIC_TRAIT && asAtomHandler::is<DisplayObject>(v->var))
	{
		DisplayObject* obj = asAtomHandler::as<DisplayObject>(v->var);
		if (!obj->legacy)
		{
			obj->incRef();
			_removeChild(obj);
		}
	}
	return InteractiveObject::deleteVariableByMultiname(name);
}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void MovieClip::declareFrame()
{
	/* Go through the list of frames.
	 * If our next_FP is after our current,
	 * we construct all frames from current
	 * to next_FP.
	 * If our next_FP is before our current,
	 * we purge all objects on the 0th frame
	 * and then construct all frames from
	 * the 0th to the next_FP.
	 * We also will run the constructor on objects that got placed and deleted
	 * before state.FP (which may get us an segfault).
	 *
	 */
	if((int)state.FP < state.last_FP)
	{
		purgeLegacyChildren();
		resetToStart();
	}

	//Declared traits must exists before legacy objects are added
	if (getClass())
		getClass()->setupDeclaredTraits(this);

	bool newFrame = (int)state.FP != state.last_FP;
	currentframeIterator=frames.end();
	if(getFramesLoaded())
	{
		std::list<Frame>::iterator iter=frames.begin();
		for(uint32_t i=0;i<=state.FP;i++)
		{
			if((int)state.FP < state.last_FP || (int)i > state.last_FP)
			{
				iter->execute(this);
			}
			if (!getSystemState()->mainClip->usesActionScript3 && i==state.FP && (!state.stop_FP || newFrame))
				currentframeIterator= iter;
			++iter;
		}
	}
	// remove all legacy objects that have not been handled in the PlaceObject/RemoveObject tags
	LegacyChildEraseDeletionMarked();
	DisplayObjectContainer::declareFrame();
}
void MovieClip::initFrame()
{
	/* Now the new legacy display objects are there, so we can also init their
	 * first frame (top-down) and call their constructors (bottom-up) */
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
		(*it)->initFrame();

	/* Set last_FP to reflect the frame that we have initialized currently.
	 * This must be set before the constructor of this MovieClip is run,
	 * or it will call initFrame(). */
	bool newFrame = (int)state.FP != state.last_FP;
	state.last_FP=state.FP;

	/* call our own constructor, if necassary */
	DisplayObject::initFrame();

	/* Run framescripts if this is a new frame. We do it at the end because our constructor
	 * may just have registered one. 
	 * if this is called from constructionComplete, the actionscript constructor was not called yet and 
	 * we can't execute the framescript of the first frame now (it will be executed in afterConstruction)
	 */
	if(this->getConstructIndicator() && newFrame && frameScripts.count(state.FP))
	{
		frameScriptToExecute=state.FP;
	}
	avm1initactionsdone = frameinitactionsdone.find(state.FP) != frameinitactionsdone.end();
	if (!avm1initactionsdone)
		frameinitactionsdone.insert(state.FP);
	state.creatingframe=false;
}

void MovieClip::executeFrameScript()
{
	auto itbind = variablebindings.begin();
	while (itbind != variablebindings.end())
	{
		asAtom v = getVariableBindingValue(getSystemState()->getStringFromUniqueId((*itbind).first));
		(*itbind).second->UpdateVariableBinding(v);
		itbind++;
	}
	if (currentframeIterator != frames.end())
	{
		currentframeIterator->AVM1executeActions(this,avm1initactionsdone);
		currentframeIterator = frames.end();
	}

	if (frameScriptToExecute != UINT32_MAX)
	{
		uint32_t f = frameScriptToExecute;
		frameScriptToExecute = UINT32_MAX;
		asAtom v=asAtomHandler::invalidAtom;
		asAtom obj = asAtomHandler::getClosureAtom(frameScripts[f]);
		asAtomHandler::callFunction(frameScripts[f],v,obj,NULL,0,false);
		ASATOM_DECREF(v);
	}
	Sprite::executeFrameScript();
	state.explicit_FP=false;
}

/* This is run in vm's thread context */
void DisplayObjectContainer::advanceFrame()
{
	std::vector<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
		(*it)->advanceFrame();
}

/* Update state.last_FP. If enough frames
 * are available, set state.FP to state.next_FP.
 * This is run in vm's thread context.
 */
void MovieClip::advanceFrame()
{
	state.creatingframe=true;
	if (frameScriptToExecute != UINT32_MAX)
	{
		// an ExecuteFrameScriptEvent was not handled yet, so we execute the script before advancing to next frame
		// this can happen if a MovieClip was constructed and has been set explicitely to another frame (by gotoAndStop etc.)
		// before returning to the event loop
		executeFrameScript();
	}
	/* A MovieClip can only have frames if
	 * 1a. It is a RootMovieClip
	 * 1b. or it is a DefineSpriteTag
	 * 2. and is exported as a subclass of MovieClip (see bindedTo)
	 */
	if((!this->is<RootMovieClip>() && fromDefineSpriteTag==UINT32_MAX)
	   || (!getClass()->isSubClass(Class<MovieClip>::getClass(getSystemState()))
		   && (getSystemState()->mainClip->usesActionScript3 || !getClass()->isSubClass(Class<AVM1MovieClip>::getClass(getSystemState())))))
	{
		DisplayObjectContainer::advanceFrame();
		declareFrame();
		return;
	}

	//If we have not yet loaded enough frames delay advancement
	if(state.next_FP>=(uint32_t)getFramesLoaded())
	{
		if(hasFinishedLoading())
		{
			LOG(LOG_ERROR,_("state.next_FP >= getFramesLoaded:")<< state.next_FP<<" "<<getFramesLoaded() <<" "<<toDebugString());
			state.next_FP = state.FP;
		}
		return;
	}

	if (state.next_FP != state.FP)
	{
		state.FP=state.next_FP;
	}
	if(!state.stop_FP && getFramesLoaded()>0)
	{
		state.next_FP=imin(state.FP+1,getFramesLoaded()-1);
		if(hasFinishedLoading() && state.FP == getFramesLoaded()-1)
			state.next_FP = 0;
	}
	// ensure the legacy objects of the current frame are created
	DisplayObjectContainer::advanceFrame();
	declareFrame();
}

void MovieClip::constructionComplete()
{
	DisplayObject::constructionComplete();

	/* If this object was 'new'ed from AS code, the first
	 * frame has not been initalized yet, so init the frame
	 * now */
	if(state.last_FP == -1)
	{
		advanceFrame();
		initFrame();
	}
}

void MovieClip::afterConstruction()
{
	// execute framescript of frame 0 after construction is completed
	// only if state.FP was not changed during construction
	if((currentframeIterator != frames.end() || frameScripts.count(0)) && state.FP == 0)
	{
		if (frameScripts.count(0))
			frameScriptToExecute = 0;
		this->incRef();
		this->getSystemState()->currentVm->prependEvent(NullRef, _MR(new (this->getSystemState()->unaccountedMemory) ExecuteFrameScriptEvent(_MR(this))));
	}
}

Frame *MovieClip::getCurrentFrame()
{
	if (state.FP >= frames.size())
	{
		LOG(LOG_ERROR,"MovieClip.getCurrentFrame invalid frame:"<<state.FP<<" "<<frames.size()<<" "<<this->toDebugString());
		throw RunTimeException("invalid current frame");
	}
	auto it = frames.begin();
	uint32_t i = 0;
	while (i < state.FP)
	{
		it++;
		i++;
	}
	return &(*it);
}

void AVM1Movie::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
}

void AVM1Movie::buildTraits(ASObject* o)
{
	//No traits
}

ASFUNCTIONBODY_ATOM(AVM1Movie,_constructor)
{
	DisplayObject::_constructor(ret,sys,obj,NULL,0);
}

void Shader::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(Shader,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Shader class is unimplemented."));
}

void BitmapDataChannel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALPHA",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::ALPHA),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BLUE",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::BLUE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREEN",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::GREEN),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RED",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::RED),CONSTANT_TRAIT);
}

unsigned int BitmapDataChannel::channelShift(uint32_t channelConstant)
{
	unsigned int shift;
	switch (channelConstant)
	{
		case BitmapDataChannel::ALPHA:
			shift = 24;
			break;
		case BitmapDataChannel::RED:
			shift = 16;
			break;
		case BitmapDataChannel::GREEN:
			shift = 8;
			break;
		case BitmapDataChannel::BLUE:
		default: // check
			shift = 0;
			break;
	}

	return shift;
}

void LineScaleMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("HORIZONTAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"horizontal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VERTICAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"vertical"),CONSTANT_TRAIT);
}

bool Stage3D::renderImpl(RenderContext &ctxt) const
{
	if (!visible || context3D.isNull())
		return false;
	return context3D->renderImpl(ctxt);
}

void Stage3D::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("requestContext3D","",Class<IFunction>::getFunction(c->getSystemState(),requestContext3D),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("requestContext3DMatchingProfiles","",Class<IFunction>::getFunction(c->getSystemState(),requestContext3DMatchingProfiles),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,x);
	REGISTER_GETTER_SETTER(c,y);
	REGISTER_GETTER_SETTER(c,visible);
	REGISTER_GETTER(c,context3D);
}
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,x);
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,y);
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,visible);
ASFUNCTIONBODY_GETTER(Stage3D,context3D);

ASFUNCTIONBODY_ATOM(Stage3D,_constructor)
{
	//Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	EventDispatcher::_constructor(ret,sys,obj,NULL,0);
}
ASFUNCTIONBODY_ATOM(Stage3D,requestContext3D)
{
	Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	tiny_string context3DRenderMode;
	tiny_string profile;
	ARG_UNPACK_ATOM(context3DRenderMode,"auto")(profile,"baseline");
	
	th->context3D = _MR(Class<Context3D>::getInstanceS(sys));
	th->context3D->driverInfo = sys->getEngineData()->driverInfoString;
	th->incRef();
	getVm(sys)->addEvent(_MR(th),_MR(Class<Event>::getInstanceS(sys,"context3DCreate")));
}
ASFUNCTIONBODY_ATOM(Stage3D,requestContext3DMatchingProfiles)
{
	//Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	_NR<Vector> profiles;
	ARG_UNPACK_ATOM(profiles);
	LOG(LOG_NOT_IMPLEMENTED,"Stage3D.requestContext3DMatchingProfiles does nothing");
}

void ActionScriptVersion::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ACTIONSCRIPT2",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ACTIONSCRIPT3",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)3),CONSTANT_TRAIT);
}
