#ifndef __XML_H_
#define __XML_H_

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <pthread.h>
//#include <cctype>
using namespace std;

class Xml
{
public:
	class Node{
	private:
		void *pnode;
		Xml *xml;

		map<string,string> *attr;
		list<Node> *children;
		Node *parent;


	public:
		friend class Xml;

		string getTitle(void);
		string getText(void);
		void getAttribute(map<string,string> *attr);
		void getChildren(list<Node> *children);
		void getParent(Xml::Node *parent);
		bool getNode(const char *title, Node *node);
		bool getNode(const char *title,const char *text,Xml::Node *node);
		
		void addChild(const char *title,const char *text);
		void setAttribute(const char *name,const char *value);
		void setText(const char *text);

		void deletedNode(void);
		void removeAttr(const char *name);
	};

private:
	string fName;
	void *pDoc;


private:
	bool open();
	void creat(const char *rootNode,const char *version,const char *encoding);
	void save(void);


public:
	Xml(const char *xmlFileName);
	~Xml();
	
	void printfile();
	void getDeclaration(string &version, string &encoding, string &standalone);
	bool getNode(const char *title,Xml::Node *node);
	bool getNode(const char *title,const char *text,Xml::Node *node);
};


#endif

